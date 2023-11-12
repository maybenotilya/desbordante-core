#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>

#include "util/intersect_sorted_sequences.h"
#include "util/py_tuple_hash.h"

namespace std {
template <>
struct hash<std::vector<algos::hymd::ValueIdentifier>> {
    std::size_t operator()(std::vector<algos::hymd::ValueIdentifier> const& p) const {
        using algos::hymd::ValueIdentifier;
        auto hasher = util::PyTupleHash<ValueIdentifier>(p.size());
        for (ValueIdentifier el : p) {
            hasher.AddValue(el);
        }
        return hasher.GetResult();
    }
};
}  // namespace std

namespace {
std::vector<model::Index> GetNonZeroIndices(algos::hymd::DecisionBoundaryVector const& lhs) {
    std::vector<model::Index> indices;
    for (model::Index i = 0; i < lhs.size(); ++i) {
        if (lhs[i] != 0) indices.push_back(i);
    }
    return indices;
}
}  // namespace

namespace algos::hymd {

std::unique_ptr<SimilarityData> SimilarityData::CreateFrom(
        indexes::CompressedRecords* compressed_records,
        std::vector<model::md::DecisionBoundary> min_similarities,
        std::vector<std::pair<model::Index, model::Index>> column_match_col_indices,
        std::vector<preprocessing::similarity_measure::SimilarityMeasure const*> const&
                sim_measures,
        bool is_null_equal_null) {
    assert(column_match_col_indices.size() == sim_measures.size());

    bool const one_table_given = compressed_records->OneTableGiven();
    size_t const col_match_number = column_match_col_indices.size();
    std::vector<std::vector<model::md::DecisionBoundary>> natural_decision_bounds{};
    natural_decision_bounds.reserve(col_match_number);
    std::vector<preprocessing::Similarity> lowest_sims;
    std::vector<indexes::SimilarityMatrix> sim_matrices{};
    sim_matrices.reserve(col_match_number);
    std::vector<indexes::SimilarityIndex> sim_indexes{};
    sim_indexes.reserve(col_match_number);
    // TODO: Parallelize, cache data info, make infrastructure for metrics like
    //  abs(left_value - right_value) / (max - min) <- max and min for every left value or for all
    //  values in a column match? Option? Memory limits?
    for (model::Index column_match_index = 0; column_match_index < col_match_number;
         ++column_match_index) {
        auto [left_col_index, right_col_index] = column_match_col_indices[column_match_index];
        preprocessing::similarity_measure::SimilarityMeasure const& measure =
                *sim_measures[column_match_index];
        auto const& left_pli = compressed_records->GetLeftRecords().GetPli(left_col_index);
        // shared_ptr for caching
        std::shared_ptr<preprocessing::DataInfo const> data_info_left =
                preprocessing::DataInfo::MakeFrom(left_pli, measure.GetArgType());
        std::shared_ptr<preprocessing::DataInfo const> data_info_right;
        auto const& right_pli = compressed_records->GetRightRecords().GetPli(right_col_index);
        if (one_table_given && left_col_index == right_col_index) {
            data_info_right = data_info_left;
        } else {
            data_info_right = preprocessing::DataInfo::MakeFrom(right_pli, measure.GetArgType());
        }
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

model::Index SimilarityData::GetLeftPliIndex(model::Index const column_match_index) const {
    return column_match_col_indices_[column_match_index].first;
}

void SimilarityData::LowerForColumnMatch(
        model::md::DecisionBoundary& threshold, model::Index const col_match,
        indexes::PliCluster const& cluster, std::unordered_set<RecordIdentifier> const& similar_records,
        std::vector<model::md::DecisionBoundary> const& gen_max_rhs,
        Recommendations* recommendations_ptr) const {
    if (threshold == 0.0) return;
    std::vector<CompressedRecord const*> cluster_records;
    cluster_records.reserve(cluster.size());
    auto const& left_records = GetLeftRecords().GetRecords();
    for (RecordIdentifier left_record_id : cluster) {
        cluster_records.push_back(&left_records[left_record_id]);
    }
    LowerForColumnMatch(threshold, col_match, cluster_records, similar_records, gen_max_rhs,
                        recommendations_ptr);
}

void SimilarityData::LowerForColumnMatch(
        model::md::DecisionBoundary& threshold, model::Index const col_match,
        std::vector<CompressedRecord const*> const& cluster,
        std::unordered_set<RecordIdentifier> const& similar_records,
        std::vector<model::md::DecisionBoundary> const& gen_max_rhs,
        Recommendations* recommendations_ptr) const {
    if (threshold == 0.0) return;
    model::md::DecisionBoundary const cur_gen_max_rhs = gen_max_rhs[col_match];
    model::md::DecisionBoundary const cur_min_sim = rhs_min_similarities_[col_match];

    auto const& right_records = GetRightRecords().GetRecords();

    // TODO: try calculating sim vecs here.
    std::unordered_map<ValueIdentifier, std::vector<CompressedRecord const*>> grouped;
    for (CompressedRecord const* left_record_ptr : cluster) {
        CompressedRecord const& left_record = *left_record_ptr;
        grouped[left_record[col_match]].push_back(&left_record);
    }
    for (auto const& [left_value_id, records_left] : grouped) {
        for (RecordIdentifier record_id_right : similar_records) {
            CompressedRecord const& right_record = right_records[record_id_right];
            ValueIdentifier const right_value_id = right_record[col_match];
            indexes::SimilarityMatrix const& sim_matrix = sim_matrices_[col_match];
            auto it_left = sim_matrix.find(left_value_id);
            // note: We are calculating a part of the sim vec here. Will there
            // be a speedup if we store the result somewhere?
            // TODO: try storing the results somewhere, deferring other sim lookups.
            if (it_left == sim_matrix.end()) {
                threshold = 0.0;
                for (CompressedRecord const* left_record_ptr : records_left) {
                    recommendations_ptr->emplace(left_record_ptr, &right_record);
                }
                return;
            }
            auto const& row = it_left->second;
            auto it_right = row.find(right_value_id);
            if (it_right == row.end()) {
                threshold = 0.0;
                for (CompressedRecord const* left_record_ptr : records_left) {
                    recommendations_ptr->emplace(left_record_ptr, &right_record);
                }
                return;
            }
            preprocessing::Similarity const record_similarity = it_right->second;

            if (record_similarity == 0.0) {
                threshold = 0.0;
                for (CompressedRecord const* left_record_ptr : records_left) {
                    recommendations_ptr->emplace(left_record_ptr, &right_record);
                }
                return;
            }
            assert(record_similarity >= cur_min_sim);
            if (threshold > record_similarity) {
                threshold = record_similarity;
                for (CompressedRecord const* left_record_ptr : records_left) {
                    recommendations_ptr->emplace(left_record_ptr, &right_record);
                }
                if (threshold <= cur_gen_max_rhs) {
                    // TODO: shouldUpdateViolations: if number of violations in a column match is
                    // less than 20, keep going
                    threshold = 0.0;
                    return;
                }
                assert(threshold >= cur_min_sim);
            }
        }
    }
}

std::unordered_set<SimilarityVector> SimilarityData::GetSimVecs(
        RecordIdentifier const left_record_id) const {
    // TODO: use the "slim" sim index to fill those instead of lookups in sim matrices
    size_t const col_match_number = GetColumnMatchNumber();
    CompressedRecord const& left_record = GetLeftRecords().GetRecords()[left_record_id];
    std::vector<std::pair<indexes::SimilarityMatrixRow const*, model::Index>> row_ptrs;
    row_ptrs.reserve(col_match_number);
    for (model::Index col_match_idx = 0; col_match_idx < col_match_number; ++col_match_idx) {
        indexes::SimilarityMatrix const& col_match_matrix = sim_matrices_[col_match_idx];
        auto it = col_match_matrix.find(left_record[col_match_idx]);
        if (it == col_match_matrix.end()) continue;
        row_ptrs.emplace_back(&it->second, col_match_idx);
    }
    if (row_ptrs.empty()) return {SimilarityVector(0.0, col_match_number)};
    std::unordered_set<SimilarityVector> sim_vecs;
    auto const& right_records = GetRightRecords().GetRecords();
    size_t const right_records_num = right_records.size();
    // for (RecordIdentifier right_record_id = 0;
    // Optimization not done in Metanome.
    for (RecordIdentifier right_record_id = (single_table_ ? left_record_id + 1 : 0);
         right_record_id < right_records_num; ++right_record_id) {
        CompressedRecord const& right_record = right_records[right_record_id];
        SimilarityVector pair_sims(col_match_number);
        for (auto [row_ptr, col_match_idx] : row_ptrs) {
            auto it = row_ptr->find(right_record[col_match_idx]);
            if (it == row_ptr->end()) continue;
            pair_sims[col_match_idx] = it->second;
        }
        sim_vecs.insert(std::move(pair_sims));
    }
    return sim_vecs;
}

SimilarityVector SimilarityData::GetSimilarityVector(CompressedRecord const& left_record,
                                                     CompressedRecord const& right_record) const {
    size_t const col_match_number = GetColumnMatchNumber();
    SimilarityVector sims(col_match_number);
    for (model::Index i = 0; i < col_match_number; ++i) {
        // TODO: not at, "get or default"
        indexes::SimilarityMatrix const& sim_matrix = sim_matrices_[i];
        auto row_it = sim_matrix.find(left_record[i]);
        if (row_it == sim_matrix.end()) {
            sims[i] = 0.0;
            continue;
        }
        indexes::SimilarityMatrixRow const& row = row_it->second;
        auto sim_it = row.find(right_record[i]);
        if (sim_it == row.end()) {
            sims[i] = 0.0;
            continue;
        }
        sims[i] = sim_it->second;
    }
    return sims;
}

std::unordered_set<RecordIdentifier> const* SimilarityData::GetSimilarRecords(
        ValueIdentifier value_id, model::md::DecisionBoundary similarity,
        model::Index column_match_index) const {
    indexes::SimilarityIndex const& sim_index = sim_indexes_[column_match_index];
    auto val_index_it = sim_index.find(value_id);
    if (val_index_it == sim_index.end()) return nullptr;
    auto const& val_index = val_index_it->second;
    auto it = val_index.lower_bound(similarity);
    if (it == val_index.end()) return nullptr;
    return &it->second;
}

[[nodiscard]] std::optional<model::md::DecisionBoundary> SimilarityData::SpecializeOneLhs(
        model::Index col_match_index, model::md::DecisionBoundary similarity) const {
    std::vector<model::md::DecisionBoundary> const& decision_bounds =
            natural_decision_bounds_[col_match_index];
    auto end_bounds = decision_bounds.end();
    auto upper = std::upper_bound(decision_bounds.begin(), end_bounds, similarity);
    if (upper == end_bounds) {
        // Does not handle the case where the highest possible decision bound is not 1.0
        // correctly, but Metanome doesn't either
        return std::nullopt;
    }
    return *upper;
}

SimilarityData::LhsData SimilarityData::GetMaxRhsDecBounds(
        DecisionBoundaryVector const& lhs_sims, Recommendations* recommendations_ptr,
        size_t min_support, DecisionBoundaryVector const& original_rhs_thresholds,
        std::vector<model::md::DecisionBoundary> const& gen_max_rhs,
        std::unordered_set<model::Index>& rhs_indices) const {
    size_t support = 0;
    size_t const col_matches = original_rhs_thresholds.size();
    size_t const rhs_indices_size = rhs_indices.size();
    DecisionBoundaryVector rhs_thresholds(col_matches, 0.0);
    for (model::Index const index : rhs_indices) {
        assert(original_rhs_thresholds[index] != 0.0);
        if (prune_nondisjoint_ && lhs_sims[index] != 0.0) continue;
        if (lhs_sims[index] == 1.0) continue;
        if (gen_max_rhs[index] == 1.0) continue;
        rhs_thresholds[index] = 1.0;
    }
    std::vector<model::Index> non_zero_indices = GetNonZeroIndices(lhs_sims);
    size_t const cardinality = non_zero_indices.size();
    if (cardinality == 0) {
        assert(column_match_col_indices_.size() == lhs_sims.size());
        assert(rhs_thresholds.size() == lowest_sims_.size());
        support = GetLeftSize() * GetRightSize();
        return {lowest_sims_, support < min_support};
    }
    if (cardinality == 1) {
        model::Index const non_zero_index = non_zero_indices[0];
        assert(!prune_nondisjoint_ || rhs_thresholds[non_zero_index] == 0.0);
        if (!prune_nondisjoint_) {
            rhs_thresholds[non_zero_index] = 0.0;
            rhs_indices.erase(non_zero_index);
        }
        std::vector<indexes::PliCluster> const& clusters =
                GetLeftRecords().GetPli(GetLeftPliIndex(non_zero_index)).GetClusters();
        for (ValueIdentifier value_id = 0; value_id < clusters.size(); ++value_id) {
            std::vector<RecordIdentifier> const& cluster = clusters[value_id];
            std::unordered_set<RecordIdentifier> const* similar_records_ptr =
                    GetSimilarRecords(value_id, lhs_sims[non_zero_index], non_zero_index);
            if (similar_records_ptr == nullptr) continue;
            std::unordered_set<RecordIdentifier> const& similar_records = *similar_records_ptr;
            support += cluster.size() * similar_records.size();
            size_t zeroes = 0;
            for (model::Index const col_match_idx : rhs_indices) {
                model::md::DecisionBoundary& cur_rhs_sim = rhs_thresholds[col_match_idx];
                LowerForColumnMatch(cur_rhs_sim, col_match_idx, cluster, similar_records,
                                    gen_max_rhs, recommendations_ptr);
                if (cur_rhs_sim == 0.0) ++zeroes;
            }
            if (zeroes == rhs_indices_size && support >= min_support)
                return {std::move(rhs_thresholds), false};
        }
        return {std::move(rhs_thresholds), support < min_support};
    } else {
        assert(!prune_nondisjoint_ || std::all_of(non_zero_indices.begin(), non_zero_indices.end(),
                                                  [&rhs_thresholds](model::Index const index) {
                                                      return rhs_thresholds[index] == 0.0;
                                                  }));
        std::map<model::Index, std::vector<model::Index>> col_col_match_mapping;
        for (model::Index col_match_index : non_zero_indices) {
            col_col_match_mapping[GetLeftPliIndex(col_match_index)].push_back(col_match_index);
        }
        std::vector<std::pair<model::Index, model::Index>> col_match_val_idx_vec;
        col_match_val_idx_vec.reserve(cardinality);
        {
            model::Index idx = 0;
            for (auto const& [pli_idx, col_match_idxs] : col_col_match_mapping) {
                for (model::Index const col_match_idx : col_match_idxs) {
                    col_match_val_idx_vec.emplace_back(col_match_idx, idx);
                }
                ++idx;
            }
        }
        auto const& left_records_info = GetLeftRecords();
        auto const& left_records = left_records_info.GetRecords();
        std::vector<indexes::PliCluster> const& first_pli =
                left_records_info.GetPli(col_col_match_mapping.begin()->first).GetClusters();
        size_t const first_pli_size = first_pli.size();
        std::unordered_map<std::vector<ValueIdentifier>, std::vector<CompressedRecord const*>>
                grouped;
        for (ValueIdentifier first_value_id = 0; first_value_id < first_pli_size;
             ++first_value_id) {
            indexes::PliCluster const& cluster = first_pli[first_value_id];
            for (RecordIdentifier record_id : cluster) {
                model::Index idx = 0;
                std::vector<ValueIdentifier> value_ids(col_col_match_mapping.size());
                std::vector<ValueIdentifier> const& record = left_records[record_id];
                for (auto const& [pli_idx, col_match_idxs] : col_col_match_mapping) {
                    value_ids[idx++] = record[pli_idx];
                }
                grouped[std::move(value_ids)].push_back(&record);
            }
        }
        for (auto const& [val_ids, cluster] : grouped) {
            using IterType = std::vector<RecordIdentifier>::const_iterator;
            using IterPair = std::pair<IterType, IterType>;
            using RecSet = std::unordered_set<RecordIdentifier>;
            std::vector<RecSet const*> rec_sets;
            rec_sets.reserve(cardinality);
            for (auto [column_match_index, val_ids_idx] : col_match_val_idx_vec) {
                std::unordered_set<RecordIdentifier> const* similar_records_ptr = GetSimilarRecords(
                        val_ids[val_ids_idx], lhs_sims[column_match_index], column_match_index);
                if (similar_records_ptr == nullptr) break;
                rec_sets.push_back(similar_records_ptr);
            }
            if (rec_sets.size() != cardinality) continue;
            std::sort(rec_sets.begin(), rec_sets.end(), [](RecSet const* p1, RecSet const* p2) {
                return p1->size() < p2->size();
            });
            RecSet similar_records;
            RecSet const& first = **rec_sets.begin();
            auto const try_add_rec = [&similar_records, check_set_begin = ++rec_sets.begin(),
                                      check_set_end = rec_sets.end()](RecordIdentifier rec) {
                for (auto it = check_set_begin; it != check_set_end; ++it) {
                    if ((**it).find(rec) == (**it).end()) {
                        return;
                    }
                }
                similar_records.insert(rec);
            };
            for (RecordIdentifier rec : first) {
                try_add_rec(rec);
            }
            if (similar_records.empty()) continue;
            support += cluster.size() * similar_records.size();
            size_t zeroes = 0;
            for (model::Index const col_match_idx : rhs_indices) {
                model::md::DecisionBoundary& cur_rhs_sim = rhs_thresholds[col_match_idx];
                LowerForColumnMatch(cur_rhs_sim, col_match_idx, cluster, similar_records,
                                    gen_max_rhs, recommendations_ptr);
                if (cur_rhs_sim == 0.0) ++zeroes;
            }
            if (zeroes == rhs_indices_size && support >= min_support)
                return {std::move(rhs_thresholds), false};
        }
        return {std::move(rhs_thresholds), support < min_support};
    }
    return {std::move(rhs_thresholds), support < min_support};
}

std::optional<model::md::DecisionBoundary> SimilarityData::GetPreviousDecisionBound(
        model::md::DecisionBoundary const lhs_sim, model::Index const col_match) const {
    std::vector<model::md::DecisionBoundary> const& bounds = natural_decision_bounds_[col_match];
    auto it = std::lower_bound(bounds.begin(), bounds.end(), lhs_sim);
    if (it == bounds.begin()) return std::nullopt;
    return *--it;
}

}  // namespace algos::hymd
