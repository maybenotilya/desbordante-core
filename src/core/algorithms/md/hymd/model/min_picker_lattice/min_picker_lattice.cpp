#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_lattice.h"

#include <cassert>

namespace algos::hymd::model {

void MinPickerLattice::Advance() {
    picked_lhs_.clear();
    root_ = MinPickerNode();
    ++cardinality;
}

void MinPickerLattice::PickMinimalMds(std::vector<MdLatticeNodeInfo>& mds) {
    for (MdLatticeNodeInfo& md : mds) {
        if (picked_lhs_.find(md.lhs_sims) != picked_lhs_.end()) continue;
        root_.TryAdd(md);
    }
}

std::vector<MdLatticeNodeInfo> MinPickerLattice::GetAll() {
    std::vector<MdLatticeNodeInfo> collected;
    SimilarityVector lhs(attribute_num_, 0.0);
    root_.GetAll(collected, lhs, 0, cardinality);
    for (auto const& md : collected) {
        bool inserted = picked_lhs_.insert(md.lhs_sims).second;
        assert(inserted);
    }
    root_ = MinPickerNode();
    return collected;
}

}
