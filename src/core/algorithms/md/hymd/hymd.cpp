#include "hymd.h"

#include <algorithm>

namespace algos::hymd {

void HyMD::ResetStateMd() {
    sim_matrices_.clear();
    sim_indexes_.clear();
    cur_level_ = 0;
    md_lattice_ = model::MdLattice(column_matches_.size());
    support_lattice_ = model::SupportLattice(column_matches_.size());
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
    } while(!done);
    return 10031;
}

bool HyMD::TraverseLattice(bool traverse_all) {
    size_t const col_matches_num = column_matches_.size();
    while (cur_level_ < md_lattice_.GetMaxLevel()) {
        for (model::LatticeNodeSims const& node : md_lattice_.GetLevel(cur_level_)) {
            md_lattice_.RemoveNode(node);
            model::SimilarityVector const& lhs_sims = node.lhs_sims;
            model::SimilarityVector const& rhs_sims = node.rhs_sims;
            std::vector<double> gen_max_rhs = md_lattice_.GetMaxValidGeneralizationRhs(lhs_sims);
            auto [new_rhs_sims, support] = md_lattice_.GetMaxRhsDecBounds(lhs_sims);
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
                std::optional<model::SimilarityVector> new_lhs_sims = SpecializeLhs(node.lhs_sims, i);
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
        ++cur_level_;
        if (!traverse_all) return false;
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
            SimSortedInfo sim_id_vec;
            for (auto const& [value_right, value_id_right] : right_mapping) {
                model::Similarity similarity = column_match.similarity_function_(value_left, value_right);
                /*
                 * Only relevant for user-supplied functions, benchmark the
                 * slowdown from this check and deal with it if it is
                 * significant.
                if (!(similarity >= 0 && similarity <= 1)) {
                    // Configuration error because bundled similarity functions
                    // are going to be correct, but the same cannot be said about
                    // user-supplied functions
                    throw config::OutOfRange("Unexpected similarity (" + std::to_string(similarity) + ")");
                }
                 */
                similarities.push_back(similarity);
                sim_matrix[value_id_left][value_id_right] = similarity;
                sim_id_vec.emplace_back(similarity, value_id_right);
            }
            std::sort(sim_id_vec.begin(), sim_id_vec.end());
            sim_index[value_id_left] = sim_id_vec;
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

}  // namespace algos
