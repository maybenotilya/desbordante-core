#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
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
        indexes::ColumnSimilarityInfo sim_info = measure.MakeIndexes(
                std::move(data_info_left), std::move(data_info_right), &right_pli.GetClusters(),
                min_similarities[column_match_index], is_null_equal_null);
        natural_decision_bounds.push_back(std::move(sim_info.natural_decision_boundaries));
        lowest_sims.push_back(sim_info.lowest_similarity);
        sim_matrices.push_back(std::move(sim_info.similarity_matrix));
        sim_indexes.push_back(std::move(sim_info.similarity_index));
    }
    return std::make_unique<SimilarityData>(
            compressed_records, std::move(min_similarities), std::move(column_match_col_indices),
            std::move(natural_decision_bounds), std::move(lowest_sims), std::move(sim_matrices),
            std::move(sim_indexes), one_table_given);
}

model::Index SimilarityData::GetLeftPliIndex(model::Index const column_match_index) const {
    return column_match_col_indices_[column_match_index].first;
}

bool SimilarityData::LowerForColumnMatch(
        WorkingInfo& working_info, indexes::PliCluster const& cluster,
        std::unordered_set<RecordIdentifier> const& similar_records) const {
    assert(!similar_records.empty());
    std::vector<CompressedRecord const*> cluster_records;
    cluster_records.reserve(cluster.size());
    auto const& left_records = GetLeftRecords().GetRecords();
    for (RecordIdentifier left_record_id : cluster) {
        cluster_records.push_back(&left_records[left_record_id]);
    }
    return LowerForColumnMatch(working_info, cluster_records, similar_records);
}

bool SimilarityData::LowerForColumnMatch(
        WorkingInfo& working_info, std::vector<CompressedRecord const*> const& cluster,
        std::unordered_set<RecordIdentifier> const& similar_records) const {
    assert(!similar_records.empty());
    assert(!cluster.empty());

    /*
    // Actually loses time on adult.csv
    if (working_info.threshold <= working_info.interestingness_boundary) {
        working_info.threshold = 0.0;
        // Metanome only aborts after grouping.
        if (working_info.EnoughViolations()) return true;
    }
    */
    std::unordered_map<ValueIdentifier, std::vector<CompressedRecord const*>> grouped(
            std::min(cluster.size(), working_info.col_match_values));
    for (CompressedRecord const* left_record_ptr : cluster) {
        CompressedRecord const& left_record = *left_record_ptr;
        grouped[left_record[working_info.index]].push_back(&left_record);
    }
    for (auto const& [left_value_id, records_left] : grouped) {
        for (RecordIdentifier record_id_right : similar_records) {
            CompressedRecord const& right_record = working_info.right_records[record_id_right];
            ValueIdentifier const right_value_id = right_record[working_info.index];
            auto it_left = working_info.sim_matrix.find(left_value_id);
            // note: We are calculating a part of the sim vec here. Will there
            // be a speedup if we store the result somewhere?
            // TODO: try storing the results somewhere, deferring other sim lookups.
            if (it_left == working_info.sim_matrix.end()) {
                working_info.threshold = 0.0;
                for (CompressedRecord const* left_record_ptr : records_left) {
                    working_info.violations.emplace_back(left_record_ptr, &right_record);
                }
                if (working_info.EnoughViolations()) {
                    return true;
                } else {
                    continue;
                }
            }
            auto const& row = it_left->second;
            auto it_right = row.find(right_value_id);
            if (it_right == row.end()) {
                working_info.threshold = 0.0;
                for (CompressedRecord const* left_record_ptr : records_left) {
                    working_info.violations.emplace_back(left_record_ptr, &right_record);
                }
                if (working_info.EnoughViolations()) {
                    return true;
                } else {
                    continue;
                }
            }
            preprocessing::Similarity const record_similarity = it_right->second;
            if (record_similarity < working_info.old_bound) {
                for (CompressedRecord const* left_record_ptr : records_left) {
                    working_info.violations.emplace_back(left_record_ptr, &right_record);
                }
            }

            if (record_similarity < working_info.threshold) {
                working_info.threshold = record_similarity;
                if (working_info.threshold <= working_info.interestingness_boundary) {
                    working_info.threshold = 0.0;
                    if (working_info.EnoughViolations()) {
                        return true;
                    } else {
                        continue;
                    }
                }
            }
        }
    }
    return false;
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

template <typename Func>
auto SimilarityData::ZeroWorking(std::vector<WorkingInfo>& working_info, Func func) {
    for (WorkingInfo& working : working_info) {
        working.threshold = 0.0;
    }
    auto res = func();
    for (WorkingInfo& working : working_info) {
        working.threshold = working.old_bound;
    }
    return res;
}

SimilarityData::ValidationResult SimilarityData::Validate(lattice::FullLattice& lattice,
                                                          lattice::ValidationInfo& info,
                                                          size_t min_support) const {
    DecisionBoundaryVector& lhs_sims = info.info->lhs_sims;
    DecisionBoundaryVector& rhs_sims = *info.info->rhs_sims;
    boost::dynamic_bitset<>& rhs_indices = info.rhs_indices;
    std::vector<model::Index> indices;
    indices.reserve(rhs_indices.count());
    for (std::size_t index = rhs_indices.find_first(); index != boost::dynamic_bitset<>::npos;
         index = rhs_indices.find_next(index)) {
        indices.push_back(index);
    }
    // TODO: investigate best order.
    std::sort(indices.begin(), indices.end(), [this](model::Index ind1, model::Index ind2) {
        return natural_decision_bounds_[ind1].size() < natural_decision_bounds_[ind2].size();
    });
    std::vector<std::pair<model::Index, model::md::DecisionBoundary>> to_specialize;
    to_specialize.reserve(rhs_indices.count());
    size_t support = 0;
    std::vector<model::Index> non_zero_indices = GetNonZeroIndices(lhs_sims);
    size_t const cardinality = non_zero_indices.size();
    auto const& right_records = GetRightRecords().GetRecords();
    if (cardinality == 0) {
        support = GetLeftSize() * GetRightSize();
        for (std::size_t index : indices) {
            model::md::DecisionBoundary& rhs_bound = rhs_sims[index];
            to_specialize.emplace_back(index, rhs_bound);
            rhs_bound = lowest_sims_[index];
        }
        return {{}, to_specialize, support < min_support};
    }
    if (cardinality == 1) {
        std::vector<std::vector<Recommendation>> violations;
        violations.reserve(rhs_indices.count());
        model::Index const non_zero_index = non_zero_indices[0];
        std::vector<WorkingInfo> working;
        if (prune_nondisjoint_) {
            for (std::size_t index : indices) {
                model::md::DecisionBoundary& rhs_ref = rhs_sims[index];
                assert(rhs_ref != 0.0);
                violations.emplace_back();
                working.emplace_back(rhs_ref, index, violations.back(), rhs_ref,
                                     GetLeftValueNum(index), right_records, sim_matrices_[index]);
            }
            std::vector<model::md::DecisionBoundary> const gen_max_rhs =
                    ZeroWorking(working, [&lattice, &lhs_sims]() {
                        return lattice.GetRhsInterestingnessBounds(lhs_sims);
                    });
            for (WorkingInfo& working_info : working) {
                working_info.interestingness_boundary = gen_max_rhs[working_info.index];
            }
            size_t const working_size = working.size();
            std::vector<indexes::PliCluster> const& clusters =
                    GetLeftRecords().GetPli(GetLeftPliIndex(non_zero_index)).GetClusters();
            size_t const clusters_size = clusters.size();
            for (ValueIdentifier value_id = 0; value_id < clusters_size; ++value_id) {
                std::vector<RecordIdentifier> const& cluster = clusters[value_id];
                std::unordered_set<RecordIdentifier> const* similar_records_ptr =
                        GetSimilarRecords(value_id, lhs_sims[non_zero_index], non_zero_index);
                if (similar_records_ptr == nullptr) continue;
                std::unordered_set<RecordIdentifier> const& similar_records = *similar_records_ptr;
                support += cluster.size() * similar_records.size();
                size_t stops = 0;
                for (WorkingInfo& working_info : working) {
                    bool const should_stop =
                            LowerForColumnMatch(working_info, cluster, similar_records);
                    if (should_stop) ++stops;
                }
                if (stops == working_size && support >= min_support) {
                    for (WorkingInfo const& working_info : working) {
                        model::Index const index = working_info.index;
                        model::md::DecisionBoundary const old_bound = working_info.old_bound;
                        assert(old_bound != 0.0);
                        to_specialize.emplace_back(index, old_bound);
                    }
                    return {std::move(violations), std::move(to_specialize), false};
                }
            }
            for (auto const& working_info : working) {
                model::Index const index = working_info.index;
                model::md::DecisionBoundary const old_bound = working_info.old_bound;
                model::md::DecisionBoundary const threshold = working_info.threshold;
                if (threshold == old_bound) continue;
                to_specialize.emplace_back(index, old_bound);
            }
            return {std::move(violations), std::move(to_specialize), support < min_support};
        } else {
            std::optional<std::pair<model::Index, model::md::DecisionBoundary>> same_rhs_as_lhs;
            for (std::size_t index : indices) {
                model::md::DecisionBoundary& rhs_ref = rhs_sims[index];
                assert(rhs_ref != 0.0);
                if (index == non_zero_index) {
                    same_rhs_as_lhs = {index, rhs_ref};
                    rhs_ref = 0.0;
                } else {
                    violations.emplace_back();
                    working.emplace_back(rhs_ref, index, violations.back(), rhs_ref,
                                         GetLeftValueNum(index), right_records,
                                         sim_matrices_[index]);
                }
            }
            std::vector<model::md::DecisionBoundary> const gen_max_rhs =
                    ZeroWorking(working, [&lattice, &lhs_sims]() {
                        return lattice.GetRhsInterestingnessBounds(lhs_sims);
                    });
            size_t const working_size = working.size();
            std::vector<indexes::PliCluster> const& clusters =
                    GetLeftRecords().GetPli(GetLeftPliIndex(non_zero_index)).GetClusters();
            size_t const clusters_size = clusters.size();
            for (ValueIdentifier value_id = 0; value_id < clusters_size; ++value_id) {
                std::vector<RecordIdentifier> const& cluster = clusters[value_id];
                std::unordered_set<RecordIdentifier> const* similar_records_ptr =
                        GetSimilarRecords(value_id, lhs_sims[non_zero_index], non_zero_index);
                if (similar_records_ptr == nullptr) continue;
                std::unordered_set<RecordIdentifier> const& similar_records = *similar_records_ptr;
                support += cluster.size() * similar_records.size();
                size_t stops = 0;
                for (WorkingInfo& working_info : working) {
                    bool const should_stop =
                            LowerForColumnMatch(working_info, cluster, similar_records);
                    if (should_stop) ++stops;
                }
                if (stops == working_size && support >= min_support) {
                    for (WorkingInfo const& working_info : working) {
                        model::Index const index = working_info.index;
                        model::md::DecisionBoundary const old_bound = working_info.old_bound;
                        assert(old_bound != 0.0);
                        to_specialize.emplace_back(index, old_bound);
                    }
                    if (same_rhs_as_lhs)
                        to_specialize.emplace_back(same_rhs_as_lhs->first, same_rhs_as_lhs->second);
                    return {std::move(violations), std::move(to_specialize), false};
                }
            }
            for (auto const& working_info : working) {
                model::Index const index = working_info.index;
                model::md::DecisionBoundary const old_bound = working_info.old_bound;
                model::md::DecisionBoundary const threshold = working_info.threshold;
                if (threshold == old_bound) continue;
                to_specialize.emplace_back(index, old_bound);
            }
            if (same_rhs_as_lhs)
                to_specialize.emplace_back(same_rhs_as_lhs->first, same_rhs_as_lhs->second);
            return {std::move(violations), std::move(to_specialize), support < min_support};
        }
    } else {
        std::vector<std::vector<Recommendation>> violations;
        violations.reserve(rhs_indices.count());
        std::vector<WorkingInfo> working;
        for (std::size_t index : indices) {
            model::md::DecisionBoundary& rhs_ref = rhs_sims[index];
            violations.emplace_back();
            working.emplace_back(rhs_ref, index, violations.back(), rhs_ref, GetLeftValueNum(index),
                                 right_records, sim_matrices_[index]);
        }
        std::vector<model::md::DecisionBoundary> const gen_max_rhs = ZeroWorking(
                working,
                [&lattice, &lhs_sims]() { return lattice.GetRhsInterestingnessBounds(lhs_sims); });
        size_t const working_size = working.size();
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
        for (ValueIdentifier first_value_id = 0; first_value_id < first_pli_size;
             ++first_value_id) {
            indexes::PliCluster const& cluster = first_pli[first_value_id];
            // TODO: try adding a perfect hasher (represent each record as a number using the number
            // of records in each column)
            // TODO: investigate why this seems to be the fastest on adult.csv (glibc 2.38)
            std::unordered_map<std::vector<ValueIdentifier>, std::vector<CompressedRecord const*>>
                    grouped{0};
            for (RecordIdentifier record_id : cluster) {
                model::Index idx = 0;
                std::vector<ValueIdentifier> value_ids(col_col_match_mapping.size());
                std::vector<ValueIdentifier> const& record = left_records[record_id];
                for (auto const& [pli_idx, col_match_idxs] : col_col_match_mapping) {
                    value_ids[idx++] = record[pli_idx];
                }
                grouped[std::move(value_ids)].push_back(&record);
            }
            for (auto const& [val_ids, cluster] : grouped) {
                using RecSet = std::unordered_set<RecordIdentifier>;
                std::vector<RecSet const*> rec_sets;
                rec_sets.reserve(cardinality);
                for (auto [column_match_index, val_ids_idx] : col_match_val_idx_vec) {
                    std::unordered_set<RecordIdentifier> const* similar_records_ptr =
                            GetSimilarRecords(val_ids[val_ids_idx], lhs_sims[column_match_index],
                                              column_match_index);
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
                size_t stops = 0;
                for (WorkingInfo& working_info : working) {
                    bool const should_stop =
                            LowerForColumnMatch(working_info, cluster, similar_records);
                    if (should_stop) ++stops;
                }
                if (stops == working_size && support >= min_support) {
                    for (WorkingInfo const& working_info : working) {
                        model::Index const index = working_info.index;
                        model::md::DecisionBoundary const old_bound = working_info.old_bound;
                        assert(old_bound != 0.0);
                        to_specialize.emplace_back(index, old_bound);
                    }
                    return {std::move(violations), std::move(to_specialize), false};
                }
            }
        }
        for (WorkingInfo const& working_info : working) {
            model::Index const index = working_info.index;
            model::md::DecisionBoundary const old_bound = working_info.old_bound;
            model::md::DecisionBoundary const threshold = working_info.threshold;
            // Optimization not done in Metanome
            if (threshold >= old_bound) continue;
            to_specialize.emplace_back(index, old_bound);
        }
        return {std::move(violations), std::move(to_specialize), support < min_support};
    }
    __builtin_unreachable();
    assert(false);
}

std::optional<model::md::DecisionBoundary> SimilarityData::GetPreviousDecisionBound(
        model::md::DecisionBoundary const lhs_sim, model::Index const col_match) const {
    std::vector<model::md::DecisionBoundary> const& bounds = natural_decision_bounds_[col_match];
    auto it = std::lower_bound(bounds.begin(), bounds.end(), lhs_sim);
    if (it == bounds.begin()) return std::nullopt;
    return *--it;
}

}  // namespace algos::hymd
