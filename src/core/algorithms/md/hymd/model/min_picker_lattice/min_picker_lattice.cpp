#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_lattice.h"

#include <cassert>

namespace algos::hymd::model {

void MinPickerLattice::Advance() {
    picked_lhs_.clear();
    root_ = MinPickerNode();
    ++cardinality;
}

void MinPickerLattice::PickMinimalMds(std::vector<LatticeNodeSims> const& mds) {
    for (LatticeNodeSims const& md : mds) {
        if (picked_lhs_.find(md.lhs_sims) != picked_lhs_.end()) continue;
        root_.TryAdd(md);
    }
}

std::vector<LatticeNodeSims> MinPickerLattice::GetAll() {
    std::vector<LatticeNodeSims> collected;
    SimilarityVector lhs(attribute_num_, 0.0);
    root_.GetAll(collected, lhs, 0, cardinality);
    for (auto const& md : collected) {
        assert(picked_lhs_.insert(md.lhs_sims).second);
    }
    return collected;
}

}
