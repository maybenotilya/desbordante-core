#pragma once

#include <cstddef>
#include <unordered_set>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "model/index.h"

namespace algos::hymd {

class Validator {
private:
    struct WorkingInfo;
    template <typename Action>
    static auto ZeroLatticeRhsAndDo(std::vector<WorkingInfo>& working_info,
                                    DecisionBoundaryVector& lattice_rhs_bounds, Action action);

    indexes::CompressedRecords const* const compressed_records_;
    std::vector<ColumnMatchInfo> const* const column_matches_info_;
    std::size_t const min_support_;
    lattice::FullLattice* const lattice_;
    bool const prune_nondisjoint_;

    [[nodiscard]] model::Index GetLeftPliIndex(model::Index const column_match_index) const {
        return (*column_matches_info_)[column_match_index].left_column_index;
    }

    indexes::DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return compressed_records_->GetLeftRecords();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const noexcept {
        return compressed_records_->GetRightRecords();
    }

    std::size_t GetLeftValueNum(model::Index const col_match_index) const {
        return GetLeftCompressor().GetPli(GetLeftPliIndex(col_match_index)).GetClusters().size();
    }

    std::size_t GetTotalPairsNum() const {
        return GetLeftCompressor().GetNumberOfRecords() * GetRightCompressor().GetNumberOfRecords();
    }

    constexpr static bool kSortIndices = false;

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

public:
    struct Result {
        using RhsToLowerInfo =
                std::tuple<model::Index, model::md::DecisionBoundary, model::md::DecisionBoundary>;
        std::vector<std::vector<Recommendation>> recommendations;
        std::vector<RhsToLowerInfo> rhss_to_lower_info;
        bool is_unsupported;
    };

    Validator(indexes::CompressedRecords const* compressed_records,
              std::vector<ColumnMatchInfo> const& column_matches_info, std::size_t min_support,
              lattice::FullLattice* lattice, bool prune_nondisjoint)
        : compressed_records_(compressed_records),
          column_matches_info_(&column_matches_info),
          min_support_(min_support),
          lattice_(lattice),
          prune_nondisjoint_(prune_nondisjoint) {}

    [[nodiscard]] Result Validate(lattice::ValidationInfo& validation_info) const;
};

}  // namespace algos::hymd
