#pragma once

#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"
#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_lattice.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/support_lattice/support_lattice.h"
#include "algorithms/md/hymd/similarity_info.h"

namespace algos::hymd {

class LatticeTraverser {
    model::DictionaryCompressor* records_left_;
    model::DictionaryCompressor* records_right_;

    model::SimilarityVector* rhs_min_similarities_;
    model::MdLattice* md_lattice_;
    model::SupportLattice* support_lattice_;
    std::unique_ptr<model::MinPickerLattice> min_picker_lattice_;

    size_t cur_level_ = 0;
    size_t min_support_ = 0;

public:
    bool TraverseLattice(bool traverse_all);

};

}  // namespace algos::hymd
