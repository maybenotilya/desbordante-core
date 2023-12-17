#pragma once

#include "algorithms/md/hymd/indexes/dictionary_compressor.h"
#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice/level_getter.h"
#include "algorithms/md/hymd/similarity_data.h"

namespace algos::hymd {

class LatticeTraverser {
private:
    SimilarityData* similarity_data_;

    lattice::FullLattice* lattice_;
    Recommendations recommendations_;

    std::unique_ptr<lattice::LevelGetter> const level_getter_;

    bool const prune_nondisjoint_;

    void LowerAndSpecialize(SimilarityData::ValidationResult& validation_result,
                            lattice::ValidationInfo& validation_info);

public:
    LatticeTraverser(SimilarityData* similarity_data, lattice::FullLattice* lattice,
                     std::unique_ptr<lattice::LevelGetter> level_getter)
        : similarity_data_(similarity_data),
          lattice_(lattice),
          level_getter_(std::move(level_getter)),
          prune_nondisjoint_(similarity_data_->ShouldPruneNondisjoint()) {}

    bool TraverseLattice(bool traverse_all);

    Recommendations TakeRecommendations() {
        return std::move(recommendations_);
    }
};

}  // namespace algos::hymd
