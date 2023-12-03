#include "algorithms/md/hymd/lattice/cardinality/min_picking_level_getter.h"

namespace algos::hymd::lattice::cardinality {

std::vector<ValidationInfo*> MinPickingLevelGetter::GetCurrentMdsInternal(
        std::vector<lattice::MdLatticeNodeInfo>& level_mds) {
    min_picker_.Reset();
    for (MdLatticeNodeInfo& md : level_mds) {
        std::unordered_set<model::Index> const& previously_picked_rhs = picked_[md.lhs_sims];
        std::unordered_set<model::Index> indices;
        DecisionBoundaryVector const& rhs = *md.rhs_sims;
        for (model::Index i = 0; i < attribute_num_; ++i) {
            if (rhs[i] != 0.0 && previously_picked_rhs.find(i) == previously_picked_rhs.end()) {
                indices.insert(i);
            }
        }
        min_picker_.AddGeneralizations(md, indices);
    }
    std::vector<lattice::ValidationInfo*> collected = min_picker_.GetAll();
    for (ValidationInfo* info : collected) {
        std::unordered_set<model::Index>& validated_indices = picked_[info->info->lhs_sims];
        for (model::Index const new_index : info->rhs_indices) {
            bool const inserted = validated_indices.insert(new_index).second;
            assert(inserted);
        }
    }
    if (collected.empty()) {
        picked_.clear();
        ++cur_level_;
    }
    return collected;
}

}  // namespace algos::hymd::lattice::cardinality
