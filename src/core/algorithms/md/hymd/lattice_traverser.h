#pragma once

#include "algorithms/md/hymd/indexes/dictionary_compressor.h"
#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice/level_getter.h"
#include "algorithms/md/hymd/similarity_data.h"

namespace algos::hymd {

class LatticeTraverser {
    SimilarityData* similarity_data_;

    lattice::FullLattice* lattice_;
    Recommendations recommendations;

    size_t min_support_;

    std::unique_ptr<lattice::LevelGetter> const level_getter_;

    bool prune_nondisjoint_ = true;

public:
    LatticeTraverser(SimilarityData* similarity_data, lattice::FullLattice* lattice,
                     size_t min_support, std::unique_ptr<lattice::LevelGetter> level_getter)
        : similarity_data_(similarity_data),
          lattice_(lattice),
          min_support_(min_support),
          level_getter_(std::move(level_getter)) {}

    bool TraverseLattice(bool traverse_all);

    Recommendations TakeRecommendations() {
        return std::move(recommendations);
    }
};

}  // namespace algos::hymd
