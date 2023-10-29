#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>

#include "algorithms/md/hymd/model/dictionary_compressor/pli_intersector.h"
#include "util/intersect_sorted_sequences.h"

namespace {
std::vector<size_t> GetNonZeroIndices(algos::hymd::model::SimilarityVector const& lhs) {
    std::vector<size_t> indices;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i] != 0) indices.push_back(i);
    }
    return indices;
}
}  // namespace

namespace algos::hymd {

std::unique_ptr<SimilarityData> SimilarityData::CreateFrom(
        model::CompressedRecords* const compressed_records,
        model::SimilarityVector min_similarities,
        std::vector<std::pair<size_t, size_t>> column_match_col_indices,
        std::vector<model::SimilarityMeasure const*> const& sim_measures, bool is_null_equal_null) {
    assert(column_match_col_indices.size() == sim_measures.size());

    bool const one_table_given = compressed_records->OneTableGiven();
    size_t const col_match_number = column_match_col_indices.size();
    std::vector<DecisionBoundsVector> natural_decision_bounds{};
    natural_decision_bounds.reserve(col_match_number);
    std::vector<model::Similarity> lowest_sims;
    std::vector<SimilarityMatrix> sim_matrices{};
    sim_matrices.reserve(col_match_number);
    std::vector<SimilarityIndex> sim_indexes{};
    sim_indexes.reserve(col_match_number);
    // TODO: Parallelize, cache data info, make infrastructure for metrics like
    //  abs(left_value - right_value) / (max - min) <- max and min for every left value or for all
    //  values in a column match? Option? Memory limits?
    for (size_t column_match_index = 0; column_match_index < col_match_number;
         ++column_match_index) {
        auto [left_col_index, right_col_index] = column_match_col_indices[column_match_index];
        model::SimilarityMeasure const& measure = *sim_measures[column_match_index];
        auto const& left_pli = compressed_records->GetLeftRecords().GetPli(left_col_index);
        // shared_ptr for caching
        std::shared_ptr<model::DataInfo const> data_info_left =
                model::DataInfo::MakeFrom(left_pli, measure.GetArgType());
        auto const& right_pli = compressed_records->GetRightRecords().GetPli(right_col_index);
        std::shared_ptr<model::DataInfo const> data_info_right =
                model::DataInfo::MakeFrom(right_pli, measure.GetArgType());
        auto [dec_bounds, lowest_similarity, sim_matrix, sim_index] = measure.MakeIndexes(
                std::move(data_info_left), std::move(data_info_right), &right_pli.GetClusters(),
                min_similarities[column_match_index], is_null_equal_null);
        natural_decision_bounds.push_back(std::move(dec_bounds));
        lowest_sims.push_back(lowest_similarity);
        sim_matrices.push_back(std::move(sim_matrix));
        sim_indexes.push_back(std::move(sim_index));
    }
    return std::make_unique<SimilarityData>(
            compressed_records, std::move(min_similarities), std::move(column_match_col_indices),
            std::move(natural_decision_bounds), std::move(lowest_sims), std::move(sim_matrices),
            std::move(sim_indexes), one_table_given);
}

std::map<size_t, std::vector<size_t>> SimilarityData::MakeColToColMatchMapping(
        std::vector<size_t> const& col_match_indices) const {
    std::map<size_t, std::vector<size_t>> mapping;
    for (size_t col_match_index : col_match_indices) {
        mapping[GetLeftPliIndex(col_match_index)].push_back(col_match_index);
    }
    return mapping;
}

size_t SimilarityData::GetLeftPliIndex(size_t column_match_index) const {
    return column_match_col_indices_[column_match_index].first;
}

void SimilarityData::LowerForColumnMatch(double& threshold, size_t col_match,
                                         PliCluster const& cluster,
                                         std::vector<size_t> const& similar_records,
                                         model::SimilarityVector const& gen_max_rhs,
                                         Recommendations* recommendations_ptr) const {
    if (threshold == 0.0) return;
    double const cur_gen_max_rhs = gen_max_rhs[col_match];
    double const cur_min_sim = rhs_min_similarities_[col_match];

    auto const& right_records = GetRightRecords().GetRecords();
    auto const& left_records = GetLeftRecords().GetRecords();

    std::unordered_map<ValueIdentifier, std::vector<RecordIdentifier>> grouped;
    for (RecordIdentifier record_id_left : cluster) {
        CompressedRecord const& left_record = left_records[record_id_left];
        grouped[left_record[col_match]].push_back(record_id_left);
    }
    for (auto const& [left_value_id, records_left] : grouped) {
        for (RecordIdentifier record_id_right : similar_records) {
            CompressedRecord const& right_record = right_records[record_id_right];
            ValueIdentifier const right_value_id = right_record[col_match];
            SimilarityMatrix const& sim_matrix = sim_matrices_[col_match];
            auto it_left = sim_matrix.find(left_value_id);
            if (it_left == sim_matrix.end()) {
                threshold = 0.0;
                for (RecordIdentifier record_id_left : records_left) {
                    recommendations_ptr->emplace(record_id_left, record_id_right);
                }
                return;
            }
            auto const& row = it_left->second;
            auto it_right = row.find(right_value_id);
            if (it_right == row.end()) {
                threshold = 0.0;
                for (RecordIdentifier record_id_left : records_left) {
                    recommendations_ptr->emplace(record_id_left, record_id_right);
                }
                return;
            }
            double record_similarity = it_right->second;

            if (record_similarity < rhs_min_similarities_[col_match]) {
                threshold = 0.0;
                for (RecordIdentifier record_id_left : records_left) {
                    recommendations_ptr->emplace(record_id_left, record_id_right);
                }
                return;
            }
            if (threshold > record_similarity) {
                threshold = record_similarity;
                for (RecordIdentifier record_id_left : records_left) {
                    recommendations_ptr->emplace(record_id_left, record_id_right);
                }
                if (!(threshold > cur_gen_max_rhs && threshold >= cur_min_sim)) {
                    // shouldUpdateViolations (if violation collection is too small, keep going) not
                    // done, idk what it does, but the algo should still work
                    threshold = 0.0;
                    return;
                }
            }
        }
    }
}

void SimilarityData::DecreaseRhsThresholds(model::SimilarityVector& rhs_thresholds,
                                           PliCluster const& cluster,
                                           std::vector<size_t> const& similar_records,
                                           model::SimilarityVector const& gen_max_rhs,
                                           Recommendations* recommendations_ptr) const {
    size_t const col_matches_size = column_match_col_indices_.size();
    for (size_t col_match = 0; col_match < col_matches_size; ++col_match) {
        LowerForColumnMatch(rhs_thresholds[col_match], col_match, cluster, similar_records,
                            gen_max_rhs, recommendations_ptr);
    }
}

DecBoundVectorUnorderedSet SimilarityData::GetSimVecs(RecordIdentifier const left_record_id) const {
    size_t const col_match_number = GetColumnMatchNumber();
    CompressedRecord const& left_record = GetLeftRecords().GetRecords()[left_record_id];
    std::vector<std::pair<SimilarityMatrixRow const*, size_t>> row_ptrs;
    row_ptrs.reserve(col_match_number);
    for (size_t col_match_idx = 0; col_match_idx < col_match_number; ++col_match_idx) {
        SimilarityMatrix const& col_match_matrix = sim_matrices_[col_match_idx];
        auto it = col_match_matrix.find(left_record[col_match_idx]);
        if (it == col_match_matrix.end()) continue;
        row_ptrs.emplace_back(&it->second, col_match_idx);
    }
    if (row_ptrs.empty()) return {DecisionBoundsVector(0.0, col_match_number)};
    DecBoundVectorUnorderedSet sim_vecs;
    auto const& right_records = GetRightRecords().GetRecords();
    size_t const right_records_num = right_records.size();
    DecisionBoundsVector sims;
    // Do they do this in Metanome? Seems obvious, but I don't see it anywhere.
    for (RecordIdentifier right_record_id = (single_table_ ? left_record_id + 1 : 0);
         right_record_id < right_records_num; ++right_record_id) {
        CompressedRecord const& right_record = right_records[right_record_id];
        DecisionBoundsVector pair_sims(col_match_number);
        for (auto [row_ptr, col_match_idx] : row_ptrs) {
            auto it = row_ptr->find(right_record[col_match_idx]);
            if (it == row_ptr->end()) {
                continue;
            }
            pair_sims[col_match_idx] = it->second;
        }
        sim_vecs.insert(std::move(pair_sims));
    }
    return sim_vecs;
}

model::SimilarityVector SimilarityData::GetSimilarityVector(size_t left_record,
                                                            size_t right_record) const {
    model::SimilarityVector sims(column_match_col_indices_.size());
    CompressedRecord left_values = GetLeftRecords().GetRecords()[left_record];
    CompressedRecord right_values = GetRightRecords().GetRecords()[right_record];
    for (size_t i = 0; i < column_match_col_indices_.size(); ++i) {
        // TODO: not at, "get or default"
        SimilarityMatrix const& sim_matrix = sim_matrices_[i];
        auto row_it = sim_matrix.find(left_values[i]);
        if (row_it == sim_matrix.end()) {
            sims[i] = 0.0;
            continue;
        }
        auto const& row = row_it->second;
        auto sim_it = row.find(right_values[i]);
        if (sim_it == row.end()) {
            sims[i] = 0.0;
            continue;
        }
        sims[i] = sim_it->second;
    }
    return sims;
}

std::vector<RecordIdentifier> SimilarityData::GetSimilarRecords(ValueIdentifier value_id,
                                                                model::Similarity similarity,
                                                                size_t column_match_index) const {
    SimilarityIndex const& sim_index = sim_indexes_[column_match_index];
    auto val_index_it = sim_index.find(value_id);
    if (val_index_it == sim_index.end()) return {};
    auto const& val_index = val_index_it->second;
    auto it = val_index.lower_bound(similarity);
    if (it == val_index.end()) return {};
    return it->second;
}

std::optional<model::SimilarityVector> SimilarityData::SpecializeLhs(
        model::SimilarityVector const& lhs, size_t col_match_index) const {
    return SpecializeLhs(lhs, col_match_index, lhs[col_match_index]);
}

std::optional<model::SimilarityVector> SimilarityData::SpecializeLhs(
        model::SimilarityVector const& lhs, size_t col_match_index,
        model::Similarity similarity) const {
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

[[nodiscard]] std::optional<model::Similarity> SimilarityData::SpecializeOneLhs(
        size_t col_match_index, model::Similarity similarity) const {
    std::vector<model::Similarity> const& decision_bounds =
            natural_decision_bounds_[col_match_index];
    auto end_bounds = decision_bounds.end();
    auto upper = std::upper_bound(decision_bounds.begin(), end_bounds, similarity);
    if (upper == end_bounds) {
        return std::nullopt;
    }
    return *upper;
}

SimilarityData::LhsData SimilarityData::GetMaxRhsDecBounds(
        model::SimilarityVector const& lhs_sims, Recommendations* recommendations_ptr,
        size_t min_support, model::SimilarityVector rhs_thresholds,
        model::SimilarityVector const& gen_max_rhs) const {
    size_t support = 0;
    std::vector<size_t> non_zero_indices = GetNonZeroIndices(lhs_sims);
    size_t const cardinality = non_zero_indices.size();
    if (cardinality == 0) {
        assert(column_match_col_indices_.size() == lhs_sims.size());
        assert(rhs_thresholds.size() == lowest_sims_.size());
        rhs_thresholds = lowest_sims_;
        support = GetLeftRecords().GetNumberOfRecords() * GetRightRecords().GetNumberOfRecords();
        return {std::move(rhs_thresholds), support};
    } else if (cardinality == 1) {
        size_t const non_zero_index = non_zero_indices[0];
        if (prune_nondisjoint_) rhs_thresholds[non_zero_index] = 0.0;
        std::vector<std::vector<size_t>> const& clusters =
                GetLeftRecords().GetPli(GetLeftPliIndex(non_zero_index)).GetClusters();
        for (size_t value_id = 0; value_id < clusters.size(); ++value_id) {
            std::vector<RecordIdentifier> const& cluster = clusters[value_id];
            std::vector<RecordIdentifier> similar_records =
                    GetSimilarRecords(value_id, lhs_sims[non_zero_index], non_zero_index);
            support += cluster.size() * similar_records.size();
            DecreaseRhsThresholds(rhs_thresholds, cluster, similar_records, gen_max_rhs,
                                  recommendations_ptr);
            if (std::all_of(rhs_thresholds.begin(), rhs_thresholds.end(),
                            [](model::Similarity val) { return val == 0.0; }) &&
                support >= min_support) {
                break;
            }
        }
    } else {
        if (prune_nondisjoint_)
            for (size_t idx : non_zero_indices) {
                rhs_thresholds[idx] = 0.0;
            }
        std::map<size_t, std::vector<size_t>> col_col_match_mapping =
                MakeColToColMatchMapping(non_zero_indices);
        std::vector<std::vector<PliCluster> const*> cluster_collections;
        std::unordered_map<size_t, size_t> col_match_to_val_ids_idx;
        {
            size_t idx = 0;
            for (auto [pli_idx, col_match_idxs] : col_col_match_mapping) {
                cluster_collections.push_back(&GetLeftRecords().GetPli(pli_idx).GetClusters());
                for (size_t col_match_idx : col_match_idxs) {
                    col_match_to_val_ids_idx[col_match_idx] = idx;
                }
                ++idx;
            }
        }
        model::PliIntersector intersector(cluster_collections);
        for (auto const& [val_ids, cluster] : intersector) {
            std::vector<std::vector<RecordIdentifier>> rec_vecs;
            rec_vecs.reserve(non_zero_indices.size());
            using IterType = std::vector<RecordIdentifier>::const_iterator;
            std::vector<std::pair<IterType, IterType>> iters;
            iters.reserve(non_zero_indices.size());
            for (size_t column_match_index : non_zero_indices) {
                rec_vecs.push_back(
                        GetSimilarRecords(val_ids[col_match_to_val_ids_idx[column_match_index]],
                                          lhs_sims[column_match_index], column_match_index));
                std::vector<RecordIdentifier> const& last_rec_vec = rec_vecs.back();
                iters.emplace_back(last_rec_vec.begin(), last_rec_vec.end());
            }
            std::vector<RecordIdentifier>& similar_records = *rec_vecs.begin();
            auto it = similar_records.begin();
            util::IntersectSortedSequences(
                    [&it](RecordIdentifier rec) {
                        *it = rec;
                        ++it;
                    },
                    iters);
            similar_records.erase(it, similar_records.end());
            support += cluster.size() * similar_records.size();
            DecreaseRhsThresholds(rhs_thresholds, cluster, similar_records, gen_max_rhs,
                                  recommendations_ptr);
            if (std::all_of(rhs_thresholds.begin(), rhs_thresholds.end(),
                            [](model::Similarity val) { return val == 0.0; }) &&
                support >= min_support) {
                break;
            }
        }
    }

    return {std::move(rhs_thresholds), support};
}

std::optional<model::Similarity> SimilarityData::GetPreviousSimilarity(
        model::Similarity const lhs_sim, size_t const col_match) const {
    DecisionBoundsVector const& bounds = natural_decision_bounds_[col_match];
    auto it = std::lower_bound(bounds.begin(), bounds.end(), lhs_sim);
    if (it == bounds.begin()) return std::nullopt;
    return *--it;
}

}  // namespace algos::hymd
