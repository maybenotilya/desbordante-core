#pragma once

#include <cstddef>
#include <unordered_set>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/invalidated_rhs.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "model/index.h"

namespace algos::hymd {

class Validator {
public:
    struct Result {
        std::vector<std::vector<Recommendation>> recommendations;
        InvalidatedRhss invalidated;
        bool is_unsupported;
    };

private:
    template <typename PairProvider>
    class SetPairProcessor;
    class OneCardPairProvider;
    class MultiCardPairProvider;

    indexes::CompressedRecords const* const compressed_records_;
    std::vector<ColumnMatchInfo> const* const column_matches_info_;
    std::size_t const min_support_;
    lattice::FullLattice* const lattice_;

    [[nodiscard]] bool Supported(std::size_t support) const noexcept {
        return support >= min_support_;
    }

    [[nodiscard]] model::Index GetLeftPliIndex(model::Index const column_match_index) const {
        return (*column_matches_info_)[column_match_index].left_column_index;
    }

    indexes::DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return compressed_records_->GetLeftCompressor();
    }

    indexes::DictionaryCompressor const& GetRightCompressor() const noexcept {
        return compressed_records_->GetRightCompressor();
    }

    std::size_t GetLeftValueNum(model::Index const col_match_index) const {
        return GetLeftCompressor().GetPli(GetLeftPliIndex(col_match_index)).GetClusters().size();
    }

    std::size_t GetTotalPairsNum() const {
        return GetLeftCompressor().GetNumberOfRecords() * GetRightCompressor().GetNumberOfRecords();
    }

    constexpr static bool kSortIndices = false;

    [[nodiscard]] indexes::RecSet const* GetSimilarRecords(ValueIdentifier value_id,
                                                           model::md::DecisionBoundary lhs_bound,
                                                           model::Index column_match_index) const;

public:
    Validator(indexes::CompressedRecords const* compressed_records,
              std::vector<ColumnMatchInfo> const& column_matches_info, std::size_t min_support,
              lattice::FullLattice* lattice)
        : compressed_records_(compressed_records),
          column_matches_info_(&column_matches_info),
          min_support_(min_support),
          lattice_(lattice) {}

    [[nodiscard]] Result Validate(lattice::ValidationInfo& validation_info) const;
};

}  // namespace algos::hymd
