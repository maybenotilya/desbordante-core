#include "algorithms/md/hymd/validator.h"

#include <cassert>
#include <functional>
#include <vector>

#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "algorithms/md/hymd/utility/java_hash.h"
#include "model/index.h"
#include "util/py_tuple_hash.h"

namespace std {
template <>
struct hash<std::vector<algos::hymd::ValueIdentifier>> {
    std::size_t operator()(std::vector<algos::hymd::ValueIdentifier> const& p) const {
        using algos::hymd::ValueIdentifier;
        constexpr bool kUseJavaHash = true;
        if constexpr (kUseJavaHash) {
            return algos::hymd::utility::HashIterable(p);
        } else {
            auto hasher = util::PyTupleHash<ValueIdentifier>(p.size());
            for (ValueIdentifier el : p) {
                hasher.AddValue(el);
            }
            return hasher.GetResult();
        }
    }
};
}  // namespace std

namespace {
std::vector<model::Index> GetNonZeroIndices(algos::hymd::DecisionBoundaryVector const& lhs) {
    std::vector<model::Index> indices;
    std::size_t const col_match_number = lhs.size();
    for (model::Index i = 0; i < col_match_number; ++i) {
        if (lhs[i] != 0) indices.push_back(i);
    }
    return indices;
}
}  // namespace

namespace algos::hymd {

struct Validator::WorkingInfo {
    std::vector<Recommendation>& recommendations;
    model::md::DecisionBoundary const old_bound;
    model::Index const index;
    model::md::DecisionBoundary current_bound;
    std::size_t const col_match_values;
    model::md::DecisionBoundary interestingness_boundary;
    std::vector<CompressedRecord> const& right_records;
    indexes::SimilarityMatrix const& similarity_matrix;
    model::Index const left_index;
    model::Index const right_index;

    bool EnoughRecommendations() const {
        return recommendations.size() >= 20;
    }

    bool ShouldStop() const {
        return current_bound == 0.0 && EnoughRecommendations();
    }

    WorkingInfo(model::md::DecisionBoundary old_bound, model::Index col_match_index,
                std::vector<Recommendation>& recommendations, std::size_t col_match_values,
                std::vector<CompressedRecord> const& right_records,
                indexes::SimilarityMatrix const& similarity_matrix, model::Index const left_index,
                model::Index const right_index)
        : recommendations(recommendations),
          old_bound(old_bound),
          index(col_match_index),
          current_bound(old_bound),
          col_match_values(col_match_values),
          right_records(right_records),
          similarity_matrix(similarity_matrix),
          left_index(left_index),
          right_index(right_index) {}
};

template <typename Collection>
bool Validator::LowerForColumnMatchNoCheck(
        WorkingInfo& working_info, std::vector<CompressedRecord const*> const& matched_records,
        Collection const& similar_records) const {
    assert(!similar_records.empty());
    assert(!matched_records.empty());

    std::unordered_map<ValueIdentifier, std::vector<CompressedRecord const*>> grouped(
            std::min(matched_records.size(), working_info.col_match_values));
    for (CompressedRecord const* left_record_ptr : matched_records) {
        grouped[left_record_ptr->operator[](working_info.left_index)].push_back(left_record_ptr);
    }
    model::md::DecisionBoundary& current_rhs_bound = working_info.current_bound;
    indexes::SimilarityMatrix const& similarity_matrix = working_info.similarity_matrix;
    for (auto const& [left_value_id, records_left] : grouped) {
        for (RecordIdentifier record_id_right : similar_records) {
            CompressedRecord const& right_record = working_info.right_records[record_id_right];
            auto add_recommendations = [&records_left, &right_record, &working_info]() {
                for (CompressedRecord const* left_record_ptr : records_left) {
                    working_info.recommendations.emplace_back(left_record_ptr, &right_record);
                }
            };
            auto const& row = similarity_matrix[left_value_id];
            ValueIdentifier const right_value_id = right_record[working_info.right_index];
            auto it_right = row.find(right_value_id);
            if (it_right == row.end()) {
                add_recommendations();
            rhs_not_valid:
                current_rhs_bound = 0.0;
                if (working_info.EnoughRecommendations()) return true;
                continue;
            }

            preprocessing::Similarity const pair_similarity = it_right->second;
            if (pair_similarity < working_info.old_bound) add_recommendations();
            if (pair_similarity < current_rhs_bound) current_rhs_bound = pair_similarity;
            if (current_rhs_bound <= working_info.interestingness_boundary) goto rhs_not_valid;
        }
    }
    return false;
}

bool Validator::LowerForColumnMatch(WorkingInfo& working_info,
                                    std::vector<CompressedRecord const*> const& matched_records,
                                    std::vector<RecordIdentifier> const& similar_records) const {
    if (working_info.ShouldStop()) return true;
    return LowerForColumnMatchNoCheck(working_info, matched_records, similar_records);
}

bool Validator::LowerForColumnMatch(
        WorkingInfo& working_info, indexes::PliCluster const& cluster,
        std::unordered_set<RecordIdentifier> const& similar_records) const {
    if (working_info.ShouldStop()) return true;

    assert(!similar_records.empty());
    std::vector<CompressedRecord const*> cluster_records;
    cluster_records.reserve(cluster.size());
    auto const& left_records = GetLeftCompressor().GetRecords();
    for (RecordIdentifier left_record_id : cluster) {
        cluster_records.push_back(&left_records[left_record_id]);
    }
    return LowerForColumnMatchNoCheck(working_info, cluster_records, similar_records);
}

template <typename Action>
auto Validator::ZeroLatticeRhsAndDo(std::vector<WorkingInfo>& working,
                                    DecisionBoundaryVector& lattice_rhs_bounds, Action action) {
    for (WorkingInfo const& working_info : working) {
        lattice_rhs_bounds[working_info.index] = 0.0;
    }
    auto result = action();
    for (WorkingInfo const& working_info : working) {
        lattice_rhs_bounds[working_info.index] = working_info.old_bound;
    }
    return result;
}

Validator::Result Validator::Validate(lattice::ValidationInfo& info) const {
    auto const& column_matches_info = *column_matches_info_;
    using model::Index, model::md::DecisionBoundary;
    DecisionBoundaryVector const& lhs_bounds = info.node_info->lhs_bounds;
    DecisionBoundaryVector& rhs_bounds = *info.node_info->rhs_bounds;
    // After a call to this method, info.rhs_indices must not be used
    boost::dynamic_bitset<>& indices_bitset = info.rhs_indices;
    std::vector<Index> non_zero_indices = GetNonZeroIndices(lhs_bounds);
    std::size_t const cardinality = non_zero_indices.size();
    std::vector<std::tuple<Index, DecisionBoundary, DecisionBoundary>> rhss_to_lower_info;
    rhss_to_lower_info.reserve(indices_bitset.count());
    if (cardinality == 0) [[unlikely]] {
        for (Index index = indices_bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = indices_bitset.find_next(index)) {
            DecisionBoundary const old_bound = rhs_bounds[index];
            DecisionBoundary const new_bound =
                    column_matches_info[index].similarity_info.lowest_similarity;
            if (old_bound == new_bound) [[unlikely]]
                continue;
            rhss_to_lower_info.emplace_back(index, old_bound, new_bound);
        }
        return {{}, std::move(rhss_to_lower_info), GetTotalPairsNum() < min_support_};
    }
    auto process_set_pairs = [&, &right_records = GetRightCompressor().GetRecords()](
                                     auto* const& cluster, auto* const& similar_records,
                                     auto get_next,
                                     boost::dynamic_bitset<> const& indices_bitset) -> Result {
        std::size_t const working_size = indices_bitset.count();
        std::size_t support = 0;
        std::vector<std::vector<Recommendation>> recommendations;
        std::vector<WorkingInfo> working;
        std::vector<Index> indices;
        indices.reserve(working_size);
        for (Index index = indices_bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = indices_bitset.find_next(index)) {
            indices.push_back(index);
        }
        if constexpr (kSortIndices) {
            // TODO: investigate best order.
            std::sort(indices.begin(), indices.end(),
                      [this, &column_matches_info](Index ind1, Index ind2) {
                          return column_matches_info[ind1].similarity_info.lhs_bounds.size() <
                                 column_matches_info[ind2].similarity_info.lhs_bounds.size();
                      });
        }
        recommendations.reserve(working_size);
        working.reserve(working_size);
        for (Index index : indices) {
            recommendations.emplace_back();
            auto const& [sim_info, left_index, right_index] = column_matches_info[index];
            working.emplace_back(rhs_bounds[index], index, recommendations.back(),
                                 GetLeftValueNum(index), right_records, sim_info.similarity_matrix,
                                 left_index, right_index);
        }
        std::vector<DecisionBoundary> const gen_max_rhs =
                ZeroLatticeRhsAndDo(working, rhs_bounds, [this, &lhs_bounds, &indices]() {
                    return lattice_->GetRhsInterestingnessBounds(lhs_bounds, indices);
                });
        for (Index i = 0; i < working_size; ++i) {
            working[i].interestingness_boundary = gen_max_rhs[i];
        }
        while (get_next()) {
            support += cluster->size() * similar_records->size();
            std::size_t stops = 0;
            for (WorkingInfo& working_info : working) {
                bool const should_stop =
                        LowerForColumnMatch(working_info, *cluster, *similar_records);
                if (should_stop) ++stops;
            }
            if (stops == working_size && support >= min_support_) {
                for (WorkingInfo const& working_info : working) {
                    Index const index = working_info.index;
                    DecisionBoundary const old_bound = working_info.old_bound;
                    DecisionBoundary const new_bound = working_info.current_bound;
                    assert(old_bound != 0.0);
                    assert(new_bound == 0.0);
                    rhss_to_lower_info.emplace_back(index, old_bound, 0.0);
                }
                return {std::move(recommendations), std::move(rhss_to_lower_info), false};
            }
        }
        for (WorkingInfo const& working_info : working) {
            Index const index = working_info.index;
            DecisionBoundary const old_bound = working_info.old_bound;
            DecisionBoundary const new_bound = working_info.current_bound;
            if (new_bound == old_bound) continue;
            rhss_to_lower_info.emplace_back(index, old_bound, new_bound);
        }
        return {std::move(recommendations), std::move(rhss_to_lower_info), support < min_support_};
    };
    if (cardinality == 1) {
        Index const non_zero_index = non_zero_indices.front();
        // Never happens when disjointedness pruning is on.
        if (!prune_nondisjoint_) {
            if (indices_bitset.test_set(non_zero_index, false)) {
                rhss_to_lower_info.emplace_back(non_zero_index, rhs_bounds[non_zero_index], 0.0);
            }
        }
        std::vector<indexes::PliCluster> const& clusters =
                GetLeftCompressor().GetPli(GetLeftPliIndex(non_zero_index)).GetClusters();
        std::unordered_set<RecordIdentifier> const* similar_records_ptr = nullptr;
        std::vector<RecordIdentifier> const* cluster_ptr = nullptr;
        DecisionBoundary const decision_boundary = lhs_bounds[non_zero_index];
        auto next_pair = [&, value_id = ValueIdentifier(-1),
                          clusters_size = clusters.size()]() mutable {
            for (++value_id; value_id != clusters_size; ++value_id) {
                similar_records_ptr =
                        GetSimilarRecords(value_id, decision_boundary, non_zero_index);
                if (similar_records_ptr != nullptr) {
                    cluster_ptr = &clusters[value_id];
                    return true;
                }
            }
            return false;
        };
        return process_set_pairs(cluster_ptr, similar_records_ptr, next_pair, indices_bitset);
    }

    std::vector<std::pair<Index, Index>> col_match_val_idx_vec;
    col_match_val_idx_vec.reserve(cardinality);

    std::map<Index, std::vector<Index>> pli_mapping;
    for (Index col_match_index : non_zero_indices) {
        pli_mapping[GetLeftPliIndex(col_match_index)].push_back(col_match_index);
    }
    auto const& left_compressor = GetLeftCompressor();
    auto map_it = pli_mapping.begin();
    auto const& [pli_idx, col_match_idxs] = *map_it;
    std::vector<indexes::PliCluster> const& first_pli =
            left_compressor.GetPli(pli_idx).GetClusters();
    for (Index const col_match_idx : col_match_idxs) {
        col_match_val_idx_vec.emplace_back(col_match_idx, 0);
    }
    std::size_t const plis_involved = pli_mapping.size();
    std::vector<Index> non_first_indices;
    non_first_indices.reserve(plis_involved - 1);
    {
        Index value_ids_index = 1;
        auto map_end = pli_mapping.end();
        for (++map_it; map_it != map_end; ++map_it, ++value_ids_index) {
            auto const& [pli_idx, col_match_idxs] = *map_it;
            non_first_indices.push_back(pli_idx);
            for (Index const col_match_idx : col_match_idxs) {
                col_match_val_idx_vec.emplace_back(col_match_idx, value_ids_index);
            }
        }
    }

    std::vector<RecordIdentifier> similar_records;
    std::vector<CompressedRecord const*> const* cluster_ptr;
    std::unordered_map<std::vector<ValueIdentifier>, std::vector<CompressedRecord const*>> grouped;
    using indexes::RecSet;
    auto next_pair = [&, end_it = grouped.end(), gr_it = grouped.begin(),
                      first_value_id = ValueIdentifier(-1),
                      rec_sets =
                              [&]() {
                                  std::vector<RecSet const*> rec_sets;
                                  rec_sets.reserve(cardinality);
                                  return rec_sets;
                              }(),
                      non_first_start = non_first_indices.begin(),
                      non_first_end = non_first_indices.end(),
                      &left_records = left_compressor.GetRecords(),
                      first_pli_size = first_pli.size(),
                      value_ids =
                              [&]() {
                                  std::vector<ValueIdentifier> value_ids;
                                  value_ids.reserve(plis_involved);
                                  return value_ids;
                              }()]() mutable {
        auto try_get_next = [&]() {
            if (++first_value_id == first_pli_size) {
                return false;
            }
            grouped.clear();
            indexes::PliCluster const& cluster = first_pli[first_value_id];
            value_ids.push_back(first_value_id);
            for (RecordIdentifier record_id : cluster) {
                std::vector<ValueIdentifier> const& record = left_records[record_id];
                for (auto ind_it = non_first_start; ind_it != non_first_end; ++ind_it) {
                    value_ids.push_back(record[*ind_it]);
                }
                grouped[value_ids].push_back(&record);
                value_ids.erase(++value_ids.begin(), value_ids.end());
            }
            value_ids.clear();
            gr_it = grouped.begin();
            end_it = grouped.end();
            return true;
        };
        similar_records.clear();
        do {
            while (gr_it != end_it) {
                auto const& [val_ids, cluster] = *gr_it;
                rec_sets.clear();
                ++gr_it;
                for (auto [column_match_index, value_ids_index] : col_match_val_idx_vec) {
                    std::unordered_set<RecordIdentifier> const* similar_records_ptr =
                            GetSimilarRecords(val_ids[value_ids_index],
                                              lhs_bounds[column_match_index], column_match_index);
                    if (similar_records_ptr == nullptr) goto no_similar_records;
                    rec_sets.push_back(similar_records_ptr);
                }
                goto matched_on_all;
            no_similar_records:
                continue;
            matched_on_all:
                std::sort(rec_sets.begin(), rec_sets.end(), [](RecSet const* p1, RecSet const* p2) {
                    return p1->size() < p2->size();
                });
                RecSet const& first = **rec_sets.begin();
                auto check_set_begin = ++rec_sets.begin();
                auto check_set_end = rec_sets.end();
                for (RecordIdentifier rec : first) {
                    if (std::all_of(check_set_begin, check_set_end, [rec](auto const& rec_set_ptr) {
                            return rec_set_ptr->contains(rec);
                        })) {
                        similar_records.push_back(rec);
                    }
                }
                if (similar_records.empty()) continue;
                cluster_ptr = &cluster;
                return true;
            }
        } while (try_get_next());
        return false;
    };
    return process_set_pairs(cluster_ptr, &similar_records, next_pair, indices_bitset);
}

}  // namespace algos::hymd
