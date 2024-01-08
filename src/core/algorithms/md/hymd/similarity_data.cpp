#include "algorithms/md/hymd/similarity_data.h"

#include <algorithm>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/utility/java_hash.h"
#include "util/py_tuple_hash.h"

namespace algos::hymd {

std::unique_ptr<SimilarityData> SimilarityData::CreateFrom(
        indexes::CompressedRecords* const compressed_records,
        std::vector<
                std::tuple<std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure>,
                           model::Index, model::Index>> const column_matches_info_initial) {
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
    return std::make_unique<SimilarityData>(compressed_records, std::move(column_matches_info));
}

std::unordered_set<SimilarityVector> SimilarityData::GetSimVecs(
        RecordIdentifier const left_record_id) const {
    // TODO: use the "slim" sim index to fill those instead of lookups in similarity matrices
    CompressedRecord const& left_record = GetLeftCompressor().GetRecords()[left_record_id];
    std::unordered_set<SimilarityVector> sim_vecs;
    // Optimization not performed in Metanome.
    RecordIdentifier const start_from = single_table_ ? left_record_id + 1 : 0;
    SimilarityVector pair_sims;
    pair_sims.reserve(GetColumnMatchNumber());
    std::vector<CompressedRecord> const& right_records = GetRightCompressor().GetRecords();
    // TODO: parallelize this
    std::for_each(right_records.begin() + start_from, right_records.end(), [&](auto const& record) {
        for (auto const& [sim_info, left_col_index, right_col_index] : column_matches_info_) {
            indexes::SimilarityMatrixRow const& sim_matrix_row =
                    sim_info.similarity_matrix[left_record[left_col_index]];
            auto it = sim_matrix_row.find(record[right_col_index]);
            pair_sims.push_back(it == sim_matrix_row.end() ? 0.0 : it->second);
        }
        sim_vecs.insert(pair_sims);
        pair_sims.clear();
    });
    return sim_vecs;
}

SimilarityVector SimilarityData::GetSimilarityVector(CompressedRecord const& left_record,
                                                     CompressedRecord const& right_record) const {
    std::size_t const col_match_number = GetColumnMatchNumber();
    SimilarityVector similarities;
    similarities.reserve(col_match_number);
    for (auto const& [sim_info, left_col_index, right_col_index] : column_matches_info_) {
        indexes::SimilarityMatrixRow const& row =
                sim_info.similarity_matrix[left_record[left_col_index]];
        auto sim_it = row.find(right_record[right_col_index]);
        similarities.push_back(sim_it == row.end() ? 0.0 : sim_it->second);
    }
    return similarities;
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

std::optional<model::md::DecisionBoundary> SimilarityData::GetPreviousDecisionBound(
        model::md::DecisionBoundary const lhs_bound, model::Index const column_match_index) const {
    std::vector<model::md::DecisionBoundary> const& bounds =
            column_matches_info_[column_match_index].similarity_info.lhs_bounds;
    auto it = std::lower_bound(bounds.begin(), bounds.end(), lhs_bound);
    if (it == bounds.begin()) return std::nullopt;
    return *--it;
}

}  // namespace algos::hymd
