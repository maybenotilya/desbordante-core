#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"

#include <cassert>

namespace algos::hymd::lattice::cardinality {

void MinPickerLattice::Reset() {
    root_ = MinPickerNode();
}

void MinPickerLattice::AddGeneralizations(MdLatticeNodeInfo& md,
                                          std::unordered_set<model::Index>& considered_indices) {
    root_.ExcludeGeneralizationRhs(md, 0, considered_indices);
    if (considered_indices.empty()) return;
    root_.RemoveSpecializations(md, 0, considered_indices);
    root_.Add(md, 0, considered_indices);
}

std::vector<ValidationInfo*> MinPickerLattice::GetAll() {
    std::vector<ValidationInfo*> collected;
    DecisionBoundaryVector lhs(attribute_num_, 0.0);
    root_.GetAll(collected);
    return collected;
}

}  // namespace algos::hymd::lattice::cardinality
