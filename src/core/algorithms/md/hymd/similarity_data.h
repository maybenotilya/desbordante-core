#pragma once

#include <optional>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/indexes/pli_cluster.h"
#include "algorithms/md/hymd/indexes/similarity_index.h"
#include "algorithms/md/hymd/indexes/similarity_matrix.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/similarity_vector.h"
#include "model/index.h"

namespace algos::hymd {

struct ColumnMatchInfo {
    indexes::ColumnMatchSimilarityInfo similarity_info;
    model::Index left_column_index;
    model::Index right_column_index;
};

class SimilarityData {
private:
    struct WorkingInfo;
    template <typename Action>
    static auto ZeroLatticeRhsAndDo(std::vector<WorkingInfo>& working_info,
                                    DecisionBoundaryVector& lattice_rhs_bounds, Action action);

    indexes::CompressedRecords* compressed_records_;

    std::vector<ColumnMatchInfo> column_matches_info_;

    bool const single_table_;

    std::size_t const min_support_;
    lattice::FullLattice* const lattice_;
    bool const prune_nondisjoint_;

    constexpr static bool kSortIndices = false;

    [[nodiscard]] model::Index GetLeftPliIndex(model::Index const column_match_index) const {
        return column_matches_info_[column_match_index].left_column_index;
    }

    indexes::DictionaryCompressor const& GetLeftCompressor() const {
        return compressed_records_->GetLeftRecords();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const {
        return compressed_records_->GetRightRecords();
    }

    std::size_t GetLeftValueNum(model::Index const col_match_index) const {
        return GetLeftCompressor().GetPli(GetLeftPliIndex(col_match_index)).GetClusters().size();
    }

    [[nodiscard]] bool LowerForColumnMatch(
            WorkingInfo& working_info, indexes::PliCluster const& cluster,
            std::unordered_set<RecordIdentifier> const& similar_records) const;
    [[nodiscard]] bool LowerForColumnMatch(
            WorkingInfo& working_info, std::vector<CompressedRecord const*> const& matched_records,
            std::vector<RecordIdentifier> const& similar_records) const;

    template <typename Collection>
    bool LowerForColumnMatchNoCheck(WorkingInfo& working_info,
                                    std::vector<CompressedRecord const*> const& matched_records,
                                    Collection const& similar_records) const;
    [[nodiscard]] std::unordered_set<RecordIdentifier> const* GetSimilarRecords(
            ValueIdentifier value_id, model::md::DecisionBoundary lhs_bound,
            model::Index column_match_index) const;

    [[nodiscard]] std::size_t GetRightSize() const {
        return GetRightCompressor().GetNumberOfRecords();
    }

public:
    struct ValidationResult {
        using RhsToLowerInfo =
                std::tuple<model::Index, model::md::DecisionBoundary, model::md::DecisionBoundary>;
        std::vector<std::vector<Recommendation>> recommendations;
        std::vector<RhsToLowerInfo> rhss_to_lower_info;
        bool is_unsupported;
    };

    SimilarityData(indexes::CompressedRecords* compressed_records,
                   std::vector<ColumnMatchInfo> column_matches_info, std::size_t min_support,
                   lattice::FullLattice* lattice, bool prune_nondisjoint)
        : compressed_records_(compressed_records),
          column_matches_info_(std::move(column_matches_info)),
          single_table_(compressed_records_->OneTableGiven()),
          min_support_(min_support),
          lattice_(lattice),
          prune_nondisjoint_(prune_nondisjoint) {}

    static std::unique_ptr<SimilarityData> CreateFrom(
            indexes::CompressedRecords* compressed_records,
            std::vector<std::tuple<
                    std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure>,
                    model::Index, model::Index>>
                    column_matches_info,
            std::size_t min_support, lattice::FullLattice* lattice, bool prune_nondisjoint);

    [[nodiscard]] bool ShouldPruneNondisjoint() const {
        return prune_nondisjoint_;
    }

    [[nodiscard]] std::size_t GetColumnMatchNumber() const {
        return column_matches_info_.size();
    }

    [[nodiscard]] std::pair<model::Index, model::Index> GetColMatchIndices(
            model::Index index) const {
        auto const& info = column_matches_info_[index];
        return {info.left_column_index, info.right_column_index};
    }

    [[nodiscard]] std::size_t GetLeftSize() const {
        return GetLeftCompressor().GetNumberOfRecords();
    }

    [[nodiscard]] std::optional<model::md::DecisionBoundary> SpecializeOneLhs(
            model::Index col_match_index, model::md::DecisionBoundary lhs_bound) const;

    [[nodiscard]] std::unordered_set<SimilarityVector> GetSimVecs(
            RecordIdentifier left_record_id) const;
    [[nodiscard]] SimilarityVector GetSimilarityVector(CompressedRecord const& left_record,
                                                       CompressedRecord const& right_record) const;

    [[nodiscard]] ValidationResult Validate(lattice::ValidationInfo& validation_info) const;

    [[nodiscard]] std::optional<model::md::DecisionBoundary> GetPreviousDecisionBound(
            model::md::DecisionBoundary lhs_bound, model::Index column_match_index) const;
};

}  // namespace algos::hymd
