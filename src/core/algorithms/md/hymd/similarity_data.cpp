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
        std::vector<std::pair<size_t, size_t>> column_match_col_indices) {
    size_t const col_match_number = column_match_col_indices.size();
    std::vector<DecisionBoundsVector> natural_decision_bounds(col_match_number);
    std::vector<SimilarityMatrix> sim_matrices(col_match_number);
    std::vector<SimilarityIndex> sim_indexes(col_match_number);
    // TODO: Parallelize, make infrastructure for metrics like
    //  abs(left_value - right_value) / (max - min) <- max and min for every left value or for all
    //  values in a column match? Option? Memory limits?
    for (size_t column_match_index = 0; column_match_index < col_match_number;
         ++column_match_index) {
        auto [left_col_index, right_col_index] = column_match_col_indices[column_match_index];
        SimilarityMatrix& sim_matrix = sim_matrices[column_match_index];
        SimilarityIndex& sim_index = sim_indexes[column_match_index];
        model::KeyedPositionListIndex const& left_pli =
                compressed_records->GetLeftRecords().GetPli(left_col_index);
        model::KeyedPositionListIndex const& right_pli =
                compressed_records->GetRightRecords().GetPli(right_col_index);
        std::vector<PliCluster> const& left_clusters = left_pli.GetClusters();
        std::vector<PliCluster> const& right_clusters = right_pli.GetClusters();
        std::vector<model::Similarity> similarities;
        similarities.reserve(left_clusters.size() * right_clusters.size());
        sim_matrix.resize(left_clusters.size());
        sim_index.resize(left_clusters.size());
        for (size_t value_id_left = 0; value_id_left < left_clusters.size(); ++value_id_left) {
            PliCluster const& left_cluster = left_clusters[value_id_left];
            RecordIdentifier left_record = left_cluster[0];
            std::vector<std::pair<double, RecordIdentifier>> sim_rec_id_vec;
            for (size_t value_id_right = 0; value_id_right < right_clusters.size();
                 ++value_id_right) {
                PliCluster const& right_cluster = right_clusters[value_id_right];
                RecordIdentifier right_record = right_cluster[0];
                model::Similarity similarity =
                        GetSimilarity(column_match_index, left_record, right_record);
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
                for (RecordIdentifier record_id : right_pli.GetClusters()[value_id_right]) {
                    sim_rec_id_vec.emplace_back(similarity, record_id);
                }
            }
            std::sort(sim_rec_id_vec.begin(), sim_rec_id_vec.end(), std::greater<>{});
            std::vector<RecordIdentifier> records;
            records.reserve(sim_rec_id_vec.size());
            for (auto [_, rec] : sim_rec_id_vec) {
                records.push_back(rec);
            }
            SimInfo sim_info;
            assert(!sim_rec_id_vec.empty());
            double previous_similarity = sim_rec_id_vec.begin()->first;
            auto const it_begin = records.begin();
            for (size_t j = 0; j < sim_rec_id_vec.size(); ++j) {
                double similarity = sim_rec_id_vec[j].first;
                if (similarity == previous_similarity) continue;
                auto const it_end = it_begin + static_cast<long>(j);
                std::sort(it_begin, it_end);
                sim_info[previous_similarity] = {it_begin, it_end};
                previous_similarity = similarity;
            }
            std::sort(records.begin(), records.end());
            sim_info[previous_similarity] = std::move(records);
            sim_index[value_id_left] = std::move(sim_info);
        }
        std::sort(similarities.begin(), similarities.end());
        similarities.erase(std::unique(similarities.begin(), similarities.end()),
                           similarities.end());
        natural_decision_bounds[column_match_index] = std::move(similarities);
    }
    return std::make_unique<SimilarityData>(
            compressed_records, std::move(min_similarities), std::move(column_match_col_indices),
            std::move(natural_decision_bounds), std::move(sim_matrices), std::move(sim_indexes));
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

std::vector<std::pair<size_t, size_t>> SimilarityData::DecreaseRhsThresholds(
        model::SimilarityVector& rhs_thresholds, PliCluster const& cluster,
        std::vector<size_t> const& similar_records) const {
    auto const col_matches_size = column_match_col_indices_.size();
    auto const& right_records = GetRightRecords().GetRecords();
    auto const& left_records = GetLeftRecords().GetRecords();
    std::vector<std::pair<size_t, size_t>> recommendations;
    for (RecordIdentifier record_id_right_ : similar_records) {
        std::vector<ValueIdentifier> const& right_record = right_records[record_id_right_];
        for (RecordIdentifier record_id_left : cluster) {
            std::vector<ValueIdentifier> const& left_record = left_records[record_id_left];
            bool recommend = false;
            bool all_zeroes = true;
            for (size_t col_match = 0; col_match < col_matches_size; ++col_match) {
                double& threshold = rhs_thresholds[col_match];
                if (threshold == 0.0) continue;
                all_zeroes = false;
                ValueIdentifier const left_value_id = left_record[col_match];
                ValueIdentifier const right_value_id = right_record[col_match];
                // TODO: not at, "get or default"
                double record_similarity =
                        sim_matrices_[col_match][left_value_id].at(right_value_id);
                if (record_similarity < rhs_min_similarities_[col_match]) record_similarity = 0.0;
                if (threshold > record_similarity) {
                    threshold = record_similarity;
                    recommend = true;
                }
            }
            if (all_zeroes) return recommendations;
            if (recommend) recommendations.emplace_back(record_id_left, record_id_right_);
        }
    }
    return recommendations;
}

model::SimilarityVector SimilarityData::GetSimilarityVector(size_t left_record,
                                                            size_t right_record) const {
    model::SimilarityVector sims(column_match_col_indices_.size());
    std::vector<ValueIdentifier> left_values = GetLeftRecords().GetRecords()[left_record];
    std::vector<ValueIdentifier> right_values = GetRightRecords().GetRecords()[right_record];
    for (size_t i = 0; i < column_match_col_indices_.size(); ++i) {
        // TODO: not at, "get or default"
        sims[i] = sim_matrices_[i][left_values[i]].at(right_values[i]);
    }
    return sims;
}

std::vector<RecordIdentifier> SimilarityData::GetSimilarRecords(ValueIdentifier value_id,
                                                                model::Similarity similarity,
                                                                size_t column_match_index) const {
    auto const& val_index = sim_indexes_[column_match_index][value_id];
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

SimilarityData::LhsData SimilarityData::GetMaxRhsDecBounds(
        model::SimilarityVector const& lhs_sims, Recommendations* recommendations_ptr) const {
    model::SimilarityVector rhs_thresholds(lhs_sims.size(), 1.0);
    size_t support = 0;
    std::vector<size_t> non_zero_indices = GetNonZeroIndices(lhs_sims);
    size_t const cardinality = non_zero_indices.size();
    Recommendations& recommendations = *recommendations_ptr;
    if (cardinality == 0) {
        assert(column_match_col_indices_.size() == lhs_sims.size());
        for (size_t i = 0; i < column_match_col_indices_.size(); ++i) {
            rhs_thresholds[i] = natural_decision_bounds_[i][0];
        }
        support = GetLeftRecords().GetNumberOfRecords() * GetRightRecords().GetNumberOfRecords();
    }
    else if (cardinality == 1) {
        size_t const non_zero_index = non_zero_indices[0];
        std::vector<std::vector<size_t>> const& clusters =
                GetLeftRecords().GetPli(GetLeftPliIndex(non_zero_index)).GetClusters();
        for (size_t value_id = 0; value_id < clusters.size(); ++value_id) {
            std::vector<RecordIdentifier> const& cluster = clusters[value_id];
            std::vector<RecordIdentifier> similar_records =
                    GetSimilarRecords(value_id, lhs_sims[non_zero_index], non_zero_index);
            support += cluster.size() * similar_records.size();
            auto this_value_recs = DecreaseRhsThresholds(rhs_thresholds, cluster, similar_records);
            recommendations.insert(recommendations.end(), this_value_recs.begin(),
                                   this_value_recs.end());
        }
    }
    else {
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
                rec_vecs.push_back(GetSimilarRecords(
                        val_ids[col_match_to_val_ids_idx[column_match_index]],
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
            auto this_value_recs = DecreaseRhsThresholds(rhs_thresholds, cluster, similar_records);
            recommendations.insert(recommendations.end(), this_value_recs.begin(),
                                   this_value_recs.end());
        }
    }

    return {std::move(rhs_thresholds), support};
}

}  // namespace algos::hymd
