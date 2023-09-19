#include "algorithms/md/hymd/hymd.h"

#include <algorithm>

#include "algorithms/md/hymd/model/dictionary_compressor/pli_intersector.h"

namespace {
size_t GetCardinality(algos::hymd::model::SimilarityVector const& lhs) {
    return std::count(lhs.begin(), lhs.end(), 0.0);
}
std::vector<size_t> GetNonZeroIndices(algos::hymd::model::SimilarityVector const& lhs) {
    std::vector<size_t> indices;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i] != 0) indices.push_back(i);
    }
    return indices;
}
void IntersectInPlace(std::set<size_t>& set1, std::set<size_t> set2) {
    auto it1 = set1.begin();
    auto it2 = set2.begin();
    while (it1 != set1.end() && it2 != set2.end()) {
        if (*it1 < *it2) {
            it1 = set1.erase(it1);
        } else if (*it2 < *it1) {
            ++it2;
        } else {
            ++it1;
            ++it2;
        }
    }
    set1.erase(it1, set1.end());
}
}  // namespace

namespace algos::hymd {

void HyMD::ResetStateMd() {
    sim_matrices_.clear();
    sim_indexes_.clear();
    cur_level_ = 0;
    md_lattice_ = model::MdLattice(column_matches_.size());
    support_lattice_ = model::SupportLattice();
    min_picker_lattice_ = model::MinPickerLattice(column_matches_.size());
    cur_record_left_ = 0;
    cur_record_right_ = 0;
    recommendations_.clear();
    checked_recommendations_.clear();
    efficiency_reciprocal_ = 100;
    natural_decision_bounds_.clear();
}

void HyMD::LoadDataInternal() {
    left_schema_ = std::make_unique<RelationalSchema>(left_table_->GetRelationName());
    for (size_t i = 0; i < left_table_->GetNumberOfColumns(); ++i) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }
    right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
    for (size_t i = 0; i < right_table_->GetNumberOfColumns(); ++i) {
        right_schema_->AppendColumn(right_table_->GetColumnName(i));
    }

    records_left_ = model::DictionaryCompressor::CreateFrom(*left_table_);
    records_right_ = model::DictionaryCompressor::CreateFrom(*right_table_);
}

unsigned long long HyMD::ExecuteInternal() {
    assert(!column_matches_.empty());
    // time stuff
    FillSimilarities();

    bool done;
    do {
        done = InferFromRecordPairs();
        done = TraverseLattice(done);
    } while (!done);
    return 10031;
}

bool HyMD::TraverseLattice(bool traverse_all) {
    size_t const col_matches_num = column_matches_.size();
    while (cur_level_ < md_lattice_.GetMaxLevel()) {
        std::vector<model::LatticeNodeSims> level_mds = md_lattice_.GetLevel(cur_level_);
        min_picker_lattice_.PickMinimalMds(level_mds);
        std::vector<model::LatticeNodeSims> cur = min_picker_lattice_.GetAll();
        if (cur.empty()) {
            ++cur_level_;
            min_picker_lattice_.Advance();
            if (!traverse_all) return cur_level_ < md_lattice_.GetMaxLevel();
            continue;
        }
        for (model::LatticeNodeSims const& node : cur) {
            md_lattice_.RemoveNode(node.lhs_sims);
            model::SimilarityVector const& lhs_sims = node.lhs_sims;
            model::SimilarityVector const& rhs_sims = node.rhs_sims;
            std::vector<double> gen_max_rhs = md_lattice_.GetMaxValidGeneralizationRhs(lhs_sims);
            auto [new_rhs_sims, support] = GetMaxRhsDecBounds(lhs_sims);
            if (support < min_support_) {
                support_lattice_.MarkUnsupported(lhs_sims);
                continue;
            }
            assert(new_rhs_sims.size() == column_matches_.size());
            for (size_t i = 0; i < new_rhs_sims.size(); ++i) {
                model::Similarity const new_rhs_sim = new_rhs_sims[i];
                if (new_rhs_sim > lhs_sims[i] && new_rhs_sim >= rhs_min_similarities_[i] &&
                    new_rhs_sim > gen_max_rhs[i]) {
                    md_lattice_.Add({lhs_sims, new_rhs_sim, i});
                }
            }
            for (size_t i = 0; i < col_matches_num; ++i) {
                std::optional<model::SimilarityVector> new_lhs_sims =
                        SpecializeLhs(node.lhs_sims, i);
                if (!new_lhs_sims.has_value()) continue;
                if (support_lattice_.IsUnsupported(new_lhs_sims.value())) continue;
                for (size_t j = 0; j < col_matches_num; ++j) {
                    model::Similarity const new_lhs_sim = new_lhs_sims.value()[j];
                    model::Similarity const rhs_sim = rhs_sims[j];
                    if (rhs_sims[j] > new_lhs_sim) {
                        md_lattice_.AddIfMin({new_lhs_sims.value(), rhs_sim, j});
                    }
                }
            }
        }
    }
    return true;
}

bool HyMD::InferFromRecordPairs() {
    size_t records_checked = 0;
    size_t mds_refined = 0;

    while (!recommendations_.empty()) {
        std::pair<size_t, size_t> rec_pair = recommendations_.back();
        recommendations_.pop_back();
        auto const [left_record, right_record] = rec_pair;
        mds_refined += CheckRecordPair(left_record, right_record);
        checked_recommendations_.emplace(rec_pair);
        if (!ShouldKeepInferring(records_checked, mds_refined)) {
            efficiency_reciprocal_ *= 2;
            return false;
        }
    }
    while (cur_record_left_ < records_left_.GetRecords().size()) {
        while (cur_record_right_ < records_right_.GetRecords().size()) {
            if (checked_recommendations_.find({cur_record_left_, cur_record_right_}) !=
                checked_recommendations_.end()) {
                ++cur_record_right_;
                continue;
            }
            mds_refined += CheckRecordPair(cur_record_left_, cur_record_right_);
            ++cur_record_right_;
            if (!ShouldKeepInferring(records_checked, mds_refined)) {
                efficiency_reciprocal_ *= 2;
                return false;
            }
        }
        ++cur_record_left_;
    }
    return true;
}

size_t HyMD::CheckRecordPair(size_t left_record, size_t right_record) {
    model::SimilarityVector sim = GetSimilarityVector(left_record, right_record);
    std::vector<model::LatticeMd> violated = md_lattice_.FindViolated(sim);
    for (model::LatticeMd const& md : violated) {
        md_lattice_.RemoveMd(md);
        size_t const rhs_index = md.rhs_index;
        model::Similarity const rec_rhs_sim = sim[rhs_index];
        model::SimilarityVector const& md_lhs = md.lhs_sims;
        if (rec_rhs_sim >= rhs_min_similarities_[rhs_index] && rec_rhs_sim > md_lhs[rhs_index]) {
            md_lattice_.AddIfMin({md.lhs_sims, rec_rhs_sim, rhs_index});
        }
        for (size_t i = 0; i < column_matches_.size(); ++i) {
            std::optional<model::SimilarityVector> const& new_lhs =
                    SpecializeLhs(md_lhs, i, sim[i]);
            if (!new_lhs.has_value()) continue;
            if (md.rhs_sim > new_lhs.value()[md.rhs_index]) {
                md_lattice_.AddIfMin({new_lhs.value(), md.rhs_sim, md.rhs_index});
            }
        }
    }
    return violated.size();
}

bool HyMD::ShouldKeepInferring(size_t records_checked, size_t mds_refined) {
    return mds_refined != 0 && records_checked / mds_refined > efficiency_reciprocal_;
}

void HyMD::FillSimilarities() {
    sim_matrices_.resize(column_matches_.size());
    sim_indexes_.resize(column_matches_.size());
    natural_decision_bounds_.resize(column_matches_.size());
    for (size_t i = 0; i < column_matches_.size(); ++i) {
        model::ColumnMatchInternal const& column_match = column_matches_[i];
        SimilarityMatrix& sim_matrix = sim_matrices_[i];
        SimilarityIndex& sim_index = sim_indexes_[i];
        std::unordered_map<std::string, size_t> const& left_mapping =
                records_left_.GetPlis()[column_match.left_col_index].GetMapping();
        std::unordered_map<std::string, size_t> const& right_mapping =
                records_right_.GetPlis()[column_match.right_col_index].GetMapping();
        std::vector<model::Similarity> similarities;
        similarities.reserve(left_mapping.size() * right_mapping.size());
        sim_matrix.resize(left_mapping.size());
        sim_index.resize(left_mapping.size());
        for (auto const& [value_left, value_id_left] : left_mapping) {
            std::vector<std::pair<double, ValueIdentifier>> sim_id_vec;
            for (auto const& [value_right, value_id_right] : right_mapping) {
                model::Similarity similarity =
                        column_match.similarity_function_(value_left, value_right);
                /*
                 * Only relevant for user-supplied functions, benchmark the
                 * slowdown from this check and deal with it if it is
                 * significant.
                if (!(similarity >= 0 && similarity <= 1)) {
                    // Configuration error because bundled similarity functions
                    // are going to be correct, but the same cannot be said about
                    // user-supplied functions
                    throw config::OutOfRange("Unexpected similarity (" + std::to_string(similarity)
                + ")");
                }
                 */
                similarities.push_back(similarity);
                sim_matrix[value_id_left][value_id_right] = similarity;
                sim_id_vec.emplace_back(similarity, value_id_right);
            }
            std::sort(sim_id_vec.begin(), sim_id_vec.end());
            std::map<model::Similarity, size_t> sim_mapping;
            std::vector<ValueIdentifier> sim_sorted_records;
            sim_sorted_records.reserve(sim_id_vec.size());
            double prev_sim = -1.0;
            for (size_t idx = 0; idx < sim_sorted_records.size(); ++idx) {
                auto [similarity, record] = sim_id_vec[idx];
                if (prev_sim != similarity) {
                    prev_sim = similarity;
                    sim_mapping.emplace(similarity, idx);
                }
                sim_sorted_records.emplace_back(similarity);
            }
            sim_index[value_id_left] = {sim_mapping, sim_sorted_records};
        }
        std::sort(similarities.begin(), similarities.end());
        natural_decision_bounds_[i] = similarities;
    }
}

model::SimilarityVector HyMD::GetSimilarityVector(size_t left_record, size_t right_record) {
    model::SimilarityVector sims(column_matches_.size());
    std::vector<ValueIdentifier> left_values = records_left_.GetRecords()[left_record];
    std::vector<ValueIdentifier> right_values = records_right_.GetRecords()[right_record];
    for (size_t i = 0; i < column_matches_.size(); ++i) {
        sims[i] = sim_matrices_[i][left_values[i]][right_values[i]];
    }
    return sims;
}

std::optional<model::SimilarityVector> HyMD::SpecializeLhs(model::SimilarityVector const& lhs,
                                                           size_t col_match_index) {
    return SpecializeLhs(lhs, col_match_index, lhs[col_match_index]);
}

std::optional<model::SimilarityVector> HyMD::SpecializeLhs(const model::SimilarityVector& lhs,
                                                           size_t col_match_index,
                                                           model::Similarity similarity) {
    assert(col_match_index < natural_decision_bounds_.size());
    std::vector<model::Similarity> const& decision_bounds =
            natural_decision_bounds_[col_match_index];
    auto upper = std::upper_bound(decision_bounds.begin(), decision_bounds.end(), similarity);
    if (upper == decision_bounds.end()) {
        return std::nullopt;
    }
    model::SimilarityVector new_lhs = lhs;
    new_lhs[col_match_index] = *upper;
    return new_lhs;
}

std::pair<model::SimilarityVector, size_t> HyMD::GetMaxRhsDecBounds(
        model::SimilarityVector const& lhs_sims) {
    model::SimilarityVector rhs_thresholds(lhs_sims.size(), 1.0);
    size_t support = 0;
    std::vector<size_t> non_zero_indices = GetNonZeroIndices(lhs_sims);
    size_t const cardinality = non_zero_indices.size();
    if (cardinality == 0) {
        assert(column_matches_.size() == lhs_sims.size());
        for (size_t i = 0; i < column_matches_.size(); ++i) {
            rhs_thresholds[i] = natural_decision_bounds_[i][0];
        }
        support = records_left_.GetNumberOfRecords() * records_right_.GetNumberOfRecords();
    }
    else if (cardinality == 1) {
        size_t const non_zero_index = non_zero_indices[0];
        std::vector<std::vector<size_t>> const& clusters =
                records_left_.GetPlis()[GetPliIndex(non_zero_index)].GetClusters();
        SimilarityIndex const& sim_index = sim_indexes_[non_zero_index];
        for (size_t value_id = 0; value_id < clusters.size(); ++value_id) {
            std::vector<size_t> const& cluster = clusters[value_id];
            std::set<size_t> similar_records =
                    GetSimilarRecords(value_id, lhs_sims[non_zero_index], sim_index);
            support += cluster.size() * similar_records.size();
            DecreaseRhsThresholds(rhs_thresholds, cluster, similar_records);
        }
    }
    else {
        std::map<size_t, std::vector<size_t>> col_col_match_mapping =
                MakeColToColMatchMapping(non_zero_indices);
        std::vector<model::KeyedPositionListIndex const*> plis;
        std::unordered_map<size_t, size_t> col_match_to_val_ids_idx;
        {
            size_t idx = 0;
            for (auto [pli_idx, col_match_idxs] : col_col_match_mapping) {
                plis.push_back(&records_left_.GetPlis()[pli_idx]);
                for (size_t col_match_idx : col_match_idxs) {
                    col_match_to_val_ids_idx[col_match_idx] = idx;
                }
                ++idx;
            }
        }
        model::PliIntersector intersector(plis);
        for (auto const& [val_ids, cluster] : intersector) {
            std::set<RecordIdentifier> similar_records =
                    GetSimilarRecords(val_ids[col_match_to_val_ids_idx[0]],
                                      lhs_sims[non_zero_indices[0]], sim_indexes_[0]);
            for (size_t i = 1; i < non_zero_indices.size(); ++i) {
                IntersectInPlace(similar_records,
                                 GetSimilarRecords(val_ids[col_match_to_val_ids_idx[i]],
                                                   lhs_sims[non_zero_indices[i]], sim_indexes_[i]));
            }
            support += cluster.size() * similar_records.size();
            DecreaseRhsThresholds(rhs_thresholds, cluster, similar_records);
        }
    }

    return {rhs_thresholds, support};
}

std::set<HyMD::RecordIdentifier> HyMD::GetSimilarRecords(ValueIdentifier value_id,
                                                         model::Similarity similarity,
                                                         SimilarityIndex const& sim_index) {
    auto const& sim_map = sim_index[value_id].first;
    auto const& records = sim_index[value_id].second;
    auto it = sim_map.lower_bound(similarity);
    if (it == sim_map.end()) return {};
    return {records.begin() + it->second, records.end()};
}

void HyMD::DecreaseRhsThresholds(model::SimilarityVector& rhs_thresholds, PliCluster const& cluster, std::set<size_t> const& similar_records) {
    for (size_t k : cluster) {
        for (size_t l : similar_records) {
            for (size_t col_match = 0; col_match < column_matches_.size(); ++col_match) {
                double const record_similarity = sim_matrices_[col_match][k][l];
                double& threshold = rhs_thresholds[col_match];
                if (threshold > record_similarity) {
                    threshold = record_similarity;
                }
            }
        }
    }
}

std::map<size_t, std::vector<size_t>> HyMD::MakeColToColMatchMapping(
        std::vector<size_t> const& col_match_indices) {
    std::map<size_t, std::vector<size_t>> mapping;
    for (size_t col_match_index : col_match_indices) {
        mapping[GetPliIndex(col_match_index)].push_back(col_match_index);
    }
    return mapping;
}

size_t HyMD::GetPliIndex(size_t column_match_index) {
    return column_matches_[column_match_index].left_col_index;
}

}  // namespace algos::hymd
