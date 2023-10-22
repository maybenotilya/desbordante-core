#pragma once

#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/full_lattice.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"
#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_lattice.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/support_lattice/support_lattice.h"
#include "algorithms/md/hymd/similarity_data.h"

namespace algos::hymd {

class LatticeTraverser {
    SimilarityData* similarity_data_;

    model::FullLattice* lattice_;
    Recommendations* recommendations_ptr_;

    size_t cur_level_ = 0;
    size_t min_support_;

    std::unique_ptr<model::MinPickerLattice> min_picker_lattice_;

public:
    LatticeTraverser(SimilarityData* similarity_data, model::FullLattice* lattice,
                     Recommendations* recommendations_ptr, size_t min_support)
        : similarity_data_(similarity_data),
          lattice_(lattice),
          recommendations_ptr_(recommendations_ptr),
          min_support_(min_support),
          min_picker_lattice_(std::make_unique<model::MinPickerLattice>(
                  similarity_data_->GetColumnMatchNumber())) {}

    bool TraverseLattice(bool traverse_all, bool prune_nondisjoint = true);
};

}  // namespace algos::hymd
