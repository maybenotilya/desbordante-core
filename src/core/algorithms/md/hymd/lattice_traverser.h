#pragma once

#include "algorithms/md/hymd/indexes/dictionary_compressor.h"
#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/similarity_data.h"

namespace algos::hymd {

class LatticeTraverser {
    SimilarityData* similarity_data_;

    lattice::FullLattice* lattice_;
    Recommendations* recommendations_ptr_;

    size_t cur_level_ = 0;
    size_t min_support_;

    std::unique_ptr<lattice::cardinality::MinPickerLattice> min_picker_lattice_;

    bool prune_nondisjoint_ = true;

public:
    LatticeTraverser(SimilarityData* similarity_data, lattice::FullLattice* lattice,
                     Recommendations* recommendations_ptr, size_t min_support)
        : similarity_data_(similarity_data),
          lattice_(lattice),
          recommendations_ptr_(recommendations_ptr),
          min_support_(min_support),
          min_picker_lattice_(std::make_unique<lattice::cardinality::MinPickerLattice>(
                  lattice_, similarity_data_->GetColumnMatchNumber())) {}

    bool TraverseLattice(bool traverse_all);
};

}  // namespace algos::hymd
