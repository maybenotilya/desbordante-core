#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/utility/java_hash.h"
#include "util/intersect_sorted_sequences.h"
#include "util/py_tuple_hash.h"

namespace std {
template <>
struct hash<std::vector<algos::hymd::ValueIdentifier>> {
    std::size_t operator()(std::vector<algos::hymd::ValueIdentifier> const& p) const {
        using algos::hymd::ValueIdentifier;
        constexpr bool kUseJavaHash = true;
        if constexpr (kUseJavaHash) {
            return utility::HashIterable(p);
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

struct SimilarityData::WorkingInfo {
    std::vector<Recommendation>& recommendations;
    model::md::DecisionBoundary const old_bound;
    model::Index const index;
    model::md::DecisionBoundary current_bound;
    std::size_t const col_match_values;
    model::md::DecisionBoundary interestingness_boundary;
    std::vector<CompressedRecord> const& right_records;
    indexes::SimilarityMatrix const& similarity_matrix;

    bool EnoughRecommendations() const {
        return recommendations.size() >= 20;
    }

    bool ShouldStop() const {
        return current_bound == 0.0 && EnoughRecommendations();
    }

    WorkingInfo(model::md::DecisionBoundary old_bound, model::Index index,
                std::vector<Recommendation>& violations, std::size_t col_match_values,
                std::vector<CompressedRecord> const& right_records,
                indexes::SimilarityMatrix const& similarity_matrix)
        : recommendations(violations),
          old_bound(old_bound),
          index(index),
          current_bound(old_bound),
          col_match_values(col_match_values),
          right_records(right_records),
          similarity_matrix(similarity_matrix) {}
};

std::unique_ptr<SimilarityData> SimilarityData::CreateFrom(
        indexes::CompressedRecords* const compressed_records,
        std::vector<
                std::tuple<std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure>,
                           model::Index, model::Index>> const column_matches_info_initial,
        std::size_t const min_support, lattice::FullLattice* const lattice,
        bool prune_nondisjoint) {
    bool const one_table_given = compressed_records->OneTableGiven();
    std::size_t const col_match_number = column_matches_info_initial.size();
    std::vector<ColumnMatchInfo> column_matches_info;
    column_matches_info.reserve(col_match_number);
    auto const& left_records = compressed_records->GetLeftRecords();
    auto const& right_records = compressed_records->GetRightRecords();
    for (auto const& [measure, left_col_index, right_col_index] : column_matches_info_initial) {
        auto const& left_pli = left_records.GetPli(left_col_index);
        // TODO: cache DataInfo.
        std::shared_ptr<preprocessing::DataInfo const> data_info_left =
                preprocessing::DataInfo::MakeFrom(left_pli, measure->GetArgType());
        std::shared_ptr<preprocessing::DataInfo const> data_info_right;
        auto const& right_pli = right_records.GetPli(right_col_index);
        if (one_table_given && left_col_index == right_col_index) {
            data_info_right = data_info_left;
        } else {
            data_info_right = preprocessing::DataInfo::MakeFrom(right_pli, measure->GetArgType());
        }
        column_matches_info.emplace_back(
                measure->MakeIndexes(std::move(data_info_left), std::move(data_info_right),
                                     right_pli.GetClusters()),
                left_col_index, right_col_index);
    }
    return std::make_unique<SimilarityData>(compressed_records, std::move(column_matches_info),
                                            min_support, lattice, prune_nondisjoint);
}

bool SimilarityData::LowerForColumnMatch(
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

bool SimilarityData::LowerForColumnMatch(
        WorkingInfo& working_info, std::vector<CompressedRecord const*> const& matched_records,
        std::vector<RecordIdentifier> const& similar_records) const {
    if (working_info.ShouldStop()) return true;
    return LowerForColumnMatchNoCheck(working_info, matched_records, similar_records);
}

template <typename Collection>
bool SimilarityData::LowerForColumnMatchNoCheck(
        WorkingInfo& working_info, std::vector<CompressedRecord const*> const& matched_records,
        Collection const& similar_records) const {
    assert(!similar_records.empty());
    assert(!matched_records.empty());

    std::unordered_map<ValueIdentifier, std::vector<CompressedRecord const*>> grouped(
            std::min(matched_records.size(), working_info.col_match_values));
    model::Index const col_match_index = working_info.index;
    for (CompressedRecord const* left_record_ptr : matched_records) {
        grouped[left_record_ptr->operator[](col_match_index)].push_back(left_record_ptr);
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
            auto it_left = similarity_matrix.find(left_value_id);
            if (it_left == similarity_matrix.end()) {
            not_in_similarity_matrix:
                add_recommendations();
            rhs_not_valid:
                current_rhs_bound = 0.0;
                if (working_info.EnoughRecommendations()) return true;
                continue;
            }
            auto const& row = it_left->second;
            ValueIdentifier const right_value_id = right_record[col_match_index];
            auto it_right = row.find(right_value_id);
            if (it_right == row.end()) goto not_in_similarity_matrix;

            preprocessing::Similarity const record_similarity = it_right->second;
            if (record_similarity < working_info.old_bound) add_recommendations();
            if (record_similarity < current_rhs_bound) current_rhs_bound = record_similarity;
            if (current_rhs_bound <= working_info.interestingness_boundary) goto rhs_not_valid;
        }
    }
    return false;
}

std::unordered_set<SimilarityVector> SimilarityData::GetSimVecs(
        RecordIdentifier const left_record_id) const {
    // TODO: use the "slim" sim index to fill those instead of lookups in similarity matrices
    std::size_t const col_match_number = GetColumnMatchNumber();
    CompressedRecord const& left_record = GetLeftCompressor().GetRecords()[left_record_id];
    std::vector<std::pair<indexes::SimilarityMatrixRow const*, model::Index>>
            similarity_matrix_row_ptrs;
    similarity_matrix_row_ptrs.reserve(col_match_number);
    for (model::Index col_match_idx = 0; col_match_idx < col_match_number; ++col_match_idx) {
        indexes::SimilarityMatrix const& col_match_matrix =
                column_matches_info_[col_match_idx].similarity_info.similarity_matrix;
        auto it = col_match_matrix.find(left_record[col_match_idx]);
        if (it == col_match_matrix.end()) continue;
        similarity_matrix_row_ptrs.emplace_back(&it->second, col_match_idx);
    }
    if (similarity_matrix_row_ptrs.empty()) return {SimilarityVector(col_match_number, 0.0)};
    std::unordered_set<SimilarityVector> sim_vecs;
    auto const& right_records = GetRightCompressor().GetRecords();
    std::size_t const right_records_num = right_records.size();
    // for (RecordIdentifier right_record_id = 0;
    // Optimization not done in Metanome.
    RecordIdentifier const start_from = single_table_ ? left_record_id + 1 : 0;
    // TODO: parallelize this
    SimilarityVector pair_sims(col_match_number, 0.0);
    for (RecordIdentifier right_record_id = start_from; right_record_id < right_records_num;
         ++right_record_id) {
        CompressedRecord const& right_record = right_records[right_record_id];
        for (auto [sim_matrix_row_ptr, col_match_index] : similarity_matrix_row_ptrs) {
            auto it = sim_matrix_row_ptr->find(right_record[col_match_index]);
            if (it == sim_matrix_row_ptr->end()) continue;
            pair_sims[col_match_index] = it->second;
        }
        sim_vecs.insert(pair_sims);
        pair_sims.assign(col_match_number, 0.0);
    }
    return sim_vecs;
}

SimilarityVector SimilarityData::GetSimilarityVector(CompressedRecord const& left_record,
                                                     CompressedRecord const& right_record) const {
    std::size_t const col_match_number = GetColumnMatchNumber();
    SimilarityVector similarities;
    similarities.reserve(col_match_number);
    for (model::Index i = 0; i < col_match_number; ++i) {
        indexes::SimilarityMatrix const& similarity_matrix =
                column_matches_info_[i].similarity_info.similarity_matrix;
        auto row_it = similarity_matrix.find(left_record[i]);
        if (row_it == similarity_matrix.end()) {
            similarities.push_back(0.0);
            continue;
        }
        indexes::SimilarityMatrixRow const& row = row_it->second;
        auto sim_it = row.find(right_record[i]);
        if (sim_it == row.end()) {
            similarities.push_back(0.0);
            continue;
        }
        similarities.push_back(sim_it->second);
    }
    return similarities;
}

std::unordered_set<RecordIdentifier> const* SimilarityData::GetSimilarRecords(
        ValueIdentifier value_id, model::md::DecisionBoundary lhs_bound,
        model::Index column_match_index) const {
    indexes::SimilarityIndex const& similarity_index =
            column_matches_info_[column_match_index].similarity_info.similarity_index;
    auto val_index_it = similarity_index.find(value_id);
    if (val_index_it == similarity_index.end()) return nullptr;
    auto const& val_index = val_index_it->second;
    auto it = val_index.lower_bound(lhs_bound);
    if (it == val_index.end()) return nullptr;
    return &it->second;
}

[[nodiscard]] std::optional<model::md::DecisionBoundary> SimilarityData::SpecializeOneLhs(
        model::Index col_match_index, model::md::DecisionBoundary lhs_bound) const {
    std::vector<model::md::DecisionBoundary> const& decision_bounds =
            column_matches_info_[col_match_index].similarity_info.lhs_bounds;
    auto end_bounds = decision_bounds.end();
    auto upper = std::upper_bound(decision_bounds.begin(), end_bounds, lhs_bound);
    if (upper == end_bounds) {
        // Does not handle the case where the highest possible decision bound is not 1.0
        // correctly, but Metanome doesn't either
        return std::nullopt;
    }
    return *upper;
}

template <typename Action>
auto SimilarityData::ZeroLatticeRhsAndDo(std::vector<WorkingInfo>& working,
                                         DecisionBoundaryVector& lattice_rhs_bounds,
                                         Action action) {
    for (WorkingInfo const& working_info : working) {
        lattice_rhs_bounds[working_info.index] = 0.0;
    }
    auto result = action();
    for (WorkingInfo const& working_info : working) {
        lattice_rhs_bounds[working_info.index] = working_info.old_bound;
    }
    return result;
}

SimilarityData::ValidationResult SimilarityData::Validate(lattice::ValidationInfo& info) const {
    // TODO: move to separate class.
    using model::Index, model::md::DecisionBoundary;
    DecisionBoundaryVector const& lhs_bounds = info.node_info->lhs_bounds;
    DecisionBoundaryVector& rhs_bounds = *info.node_info->rhs_bounds;
    // After a call to this method, info.rhs_indices must not be used
    boost::dynamic_bitset<>& indices_bitset = info.rhs_indices;
    std::vector<Index> non_zero_indices = GetNonZeroIndices(lhs_bounds);
    std::size_t const cardinality = non_zero_indices.size();
    std::vector<std::tuple<Index, DecisionBoundary, DecisionBoundary>> rhss_to_lower_info;
    rhss_to_lower_info.reserve(indices_bitset.count());
    if (cardinality == 0) {
        for (Index index = indices_bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = indices_bitset.find_next(index)) {
            DecisionBoundary const old_bound = rhs_bounds[index];
            DecisionBoundary const new_bound =
                    column_matches_info_[index].similarity_info.lowest_similarity;
            if (old_bound == new_bound) [[unlikely]]
                continue;
            rhss_to_lower_info.emplace_back(index, old_bound, new_bound);
        }
        return {{}, std::move(rhss_to_lower_info), GetLeftSize() * GetRightSize() < min_support_};
    }
    std::size_t support = 0;
    std::vector<std::vector<Recommendation>> recommendations;
    std::vector<WorkingInfo> working;
    std::size_t working_size;
    auto prepare = [this, &recommendations, &working, &rhs_bounds,
                    &right_records = GetRightCompressor().GetRecords(), &lhs_bounds,
                    &working_size](boost::dynamic_bitset<> const& indices_bitset) {
        working_size = indices_bitset.count();
        std::vector<Index> indices;
        indices.reserve(working_size);
        for (Index index = indices_bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = indices_bitset.find_next(index)) {
            indices.push_back(index);
        }
        if constexpr (kSortIndices) {
            // TODO: investigate best order.
            std::sort(indices.begin(), indices.end(), [this](Index ind1, Index ind2) {
                return column_matches_info_[ind1].similarity_info.lhs_bounds.size() <
                       column_matches_info_[ind2].similarity_info.lhs_bounds.size();
            });
        }
        recommendations.reserve(working_size);
        working.reserve(working_size);
        for (Index index : indices) {
            recommendations.emplace_back();
            working.emplace_back(rhs_bounds[index], index, recommendations.back(),
                                 GetLeftValueNum(index), right_records,
                                 column_matches_info_[index].similarity_info.similarity_matrix);
        }
        std::vector<DecisionBoundary> const gen_max_rhs =
                ZeroLatticeRhsAndDo(working, rhs_bounds, [this, &lhs_bounds, &indices]() {
                    return lattice_->GetRhsInterestingnessBounds(lhs_bounds, indices);
                });
        for (Index i = 0; i < working_size; ++i) {
            working[i].interestingness_boundary = gen_max_rhs[i];
        }
    };
    auto lower_bounds = [&](auto const& cluster, auto const& similar_records) {
        support += cluster.size() * similar_records.size();
        std::size_t stops = 0;
        for (WorkingInfo& working_info : working) {
            bool const should_stop = LowerForColumnMatch(working_info, cluster, similar_records);
            if (should_stop) ++stops;
        }
        return stops == working_size && support >= min_support_;
    };
    auto make_all_invalid_result = [&]() {
        for (WorkingInfo const& working_info : working) {
            Index const index = working_info.index;
            DecisionBoundary const old_bound = working_info.old_bound;
            DecisionBoundary const new_bound = working_info.current_bound;
            assert(old_bound != 0.0);
            assert(new_bound == 0.0);
            rhss_to_lower_info.emplace_back(index, old_bound, 0.0);
        }
        return ValidationResult{std::move(recommendations), std::move(rhss_to_lower_info), false};
    };
    auto make_out_of_clusters_result = [&]() {
        for (auto const& working_info : working) {
            Index const index = working_info.index;
            DecisionBoundary const old_bound = working_info.old_bound;
            DecisionBoundary const new_bound = working_info.current_bound;
            if (new_bound == old_bound) continue;
            rhss_to_lower_info.emplace_back(index, old_bound, new_bound);
        }
        return ValidationResult{std::move(recommendations), std::move(rhss_to_lower_info),
                                support < min_support_};
    };
    // TODO: figure out a way to do a loop until (cluster, similar_records) pairs end generically
    //  without extra actions
    if (cardinality == 1) {
        Index const non_zero_index = non_zero_indices.front();
        // Never happens when disjointedness pruning is on.
        if (!prune_nondisjoint_) {
            if (indices_bitset.test_set(non_zero_index, false)) {
                rhss_to_lower_info.emplace_back(non_zero_index, rhs_bounds[non_zero_index], 0.0);
            }
        }
        prepare(indices_bitset);
        std::vector<indexes::PliCluster> const& clusters =
                GetLeftCompressor().GetPli(GetLeftPliIndex(non_zero_index)).GetClusters();
        std::size_t const clusters_size = clusters.size();
        for (ValueIdentifier value_id = 0; value_id < clusters_size; ++value_id) {
            std::unordered_set<RecordIdentifier> const* similar_records_ptr =
                    GetSimilarRecords(value_id, lhs_bounds[non_zero_index], non_zero_index);
            if (similar_records_ptr == nullptr) continue;
            std::unordered_set<RecordIdentifier> const& similar_records = *similar_records_ptr;
            std::vector<RecordIdentifier> const& cluster = clusters[value_id];
            if (lower_bounds(cluster, similar_records)) {
                return make_all_invalid_result();
            }
        }
        return make_out_of_clusters_result();
    }
    prepare(indices_bitset);
    std::map<Index, std::vector<Index>> pli_mapping;
    for (Index col_match_index : non_zero_indices) {
        pli_mapping[GetLeftPliIndex(col_match_index)].push_back(col_match_index);
    }
    std::vector<std::pair<Index, Index>> col_match_val_idx_vec;
    col_match_val_idx_vec.reserve(cardinality);
    {
        Index value_ids_index = 0;
        for (auto const& [pli_idx, col_match_idxs] : pli_mapping) {
            for (Index const col_match_idx : col_match_idxs) {
                col_match_val_idx_vec.emplace_back(col_match_idx, value_ids_index);
            }
            ++value_ids_index;
        }
    }
    auto const& left_compressor = GetLeftCompressor();
    auto const& left_records = left_compressor.GetRecords();
    std::vector<indexes::PliCluster> const& first_pli =
            left_compressor.GetPli(pli_mapping.begin()->first).GetClusters();
    std::size_t const first_pli_size = first_pli.size();
    std::vector<ValueIdentifier> value_ids;
    value_ids.reserve(pli_mapping.size());
    auto pli_map_start = ++pli_mapping.begin();
    auto pli_map_end = pli_mapping.end();
    std::vector<RecordIdentifier> similar_records;
    for (ValueIdentifier first_value_id = 0; first_value_id < first_pli_size; ++first_value_id) {
        indexes::PliCluster const& cluster = first_pli[first_value_id];
        // TODO: try adding a perfect hasher (use the number of values in each column, the first
        //  value can be ignored)
        // TODO: investigate why this constructor seems to be the fastest on adult.csv
        //  (glibc 2.38)
        std::unordered_map<std::vector<ValueIdentifier>, std::vector<CompressedRecord const*>>
                grouped{0};
        {
            value_ids.push_back(first_value_id);
            for (RecordIdentifier record_id : cluster) {
                std::vector<ValueIdentifier> const& record = left_records[record_id];
                for (auto it_map = pli_map_start; it_map != pli_map_end; ++it_map) {
                    value_ids.push_back(record[it_map->first]);
                }
                grouped[value_ids].push_back(&record);
                value_ids.erase(++value_ids.begin(), value_ids.end());
            }
            value_ids.clear();
        }
        for (auto const& [val_ids, cluster] : grouped) {
            using RecSet = std::unordered_set<RecordIdentifier>;
            std::vector<RecSet const*> rec_sets;
            rec_sets.reserve(cardinality);
            for (auto [column_match_index, value_ids_index] : col_match_val_idx_vec) {
                std::unordered_set<RecordIdentifier> const* similar_records_ptr =
                        GetSimilarRecords(val_ids[value_ids_index], lhs_bounds[column_match_index],
                                          column_match_index);
                if (similar_records_ptr == nullptr) goto no_similar_records;
                rec_sets.push_back(similar_records_ptr);
            }
            goto matched_on_all;
        no_similar_records:
            continue;
        matched_on_all:
            std::sort(rec_sets.begin(), rec_sets.end(),
                      [](RecSet const* p1, RecSet const* p2) { return p1->size() < p2->size(); });
            RecSet const& first = **rec_sets.begin();
            auto check_set_begin = ++rec_sets.begin();
            auto check_set_end = rec_sets.end();
            for (RecordIdentifier rec : first) {
                if (std::none_of(check_set_begin, check_set_end, [rec](auto const*& rec_set_ptr) {
                        RecSet const& rec_set = *rec_set_ptr;
                        return rec_set.find(rec) == rec_set.end();
                    })) {
                    similar_records.push_back(rec);
                }
            }
            if (similar_records.empty()) continue;
            if (lower_bounds(cluster, similar_records)) {
                return make_all_invalid_result();
            }
            similar_records.clear();
        }
    }
    return make_out_of_clusters_result();
}

std::optional<model::md::DecisionBoundary> SimilarityData::GetPreviousDecisionBound(
        model::md::DecisionBoundary const lhs_bound, model::Index const column_match_index) const {
    std::vector<model::md::DecisionBoundary> const& bounds =
            column_matches_info_[column_match_index].similarity_info.lhs_bounds;
    auto it = std::lower_bound(bounds.begin(), bounds.end(), lhs_bound);
    if (it == bounds.begin()) return std::nullopt;
    return *--it;
}

}  // namespace algos::hymd
