#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"

#include <cassert>

namespace algos::hymd::lattice::cardinality {

void MinPickerLattice::Advance() {
    picked_.clear();
    root_ = MinPickerNode();
    ++cardinality;
}

void MinPickerLattice::PickMinimalMds(std::vector<MdLatticeNodeInfo>& mds) {
    root_ = MinPickerNode();
    for (MdLatticeNodeInfo& md : mds) {
        std::unordered_set<model::Index> const& previously_picked_rhs = picked_[md.lhs_sims];
        std::unordered_set<model::Index> indices;
        DecisionBoundaryVector const& rhs = *md.rhs_sims;
        for (model::Index i = 0; i < attribute_num_; ++i) {
            if (rhs[i] != 0.0 && previously_picked_rhs.find(i) == previously_picked_rhs.end()) {
                indices.insert(i);
            }
        }
        root_.ExcludeGeneralizationRhs(md, 0, indices);
        if (indices.empty()) continue;
        root_.RemoveSpecializations(md, 0, indices);
        root_.Add(md, 0, indices);
    }
}

std::vector<ValidationInfo*> MinPickerLattice::GetAll() {
    std::vector<ValidationInfo*> collected;
    DecisionBoundaryVector lhs(attribute_num_, 0.0);
    root_.GetAll(collected, cardinality);
    for (ValidationInfo* info : collected) {
        std::unordered_set<model::Index>& validated_indices = picked_[info->info->lhs_sims];
        for (model::Index const new_index : info->rhs_indices) {
            bool const inserted = validated_indices.insert(new_index).second;
            assert(inserted);
        }
    }
    return collected;
}

std::vector<ValidationInfo*> MinPickerLattice::GetUncheckedLevelMds(
        std::vector<MdLatticeNodeInfo>& mds) {
    PickMinimalMds(mds);
    std::vector<ValidationInfo*> unchecked = GetAll();
    if (unchecked.empty()) Advance();
    return unchecked;
}

}  // namespace algos::hymd::lattice::cardinality
