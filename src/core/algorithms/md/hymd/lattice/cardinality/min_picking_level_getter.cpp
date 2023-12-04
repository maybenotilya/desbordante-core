#include "algorithms/md/hymd/lattice/cardinality/min_picking_level_getter.h"

#include "util/erase_if_replace.h"

namespace algos::hymd::lattice::cardinality {

std::vector<ValidationInfo> MinPickingLevelGetter::GetCurrentMdsInternal(
        std::vector<MdLatticeNodeInfo>& level_mds) {
    min_picker_.NewBatch(level_mds.size());
    for (MdLatticeNodeInfo& md : level_mds) {
        DecisionBoundaryVector const& lhs_sims = md.lhs_sims;
        std::size_t const col_matches = lhs_sims.size();
        boost::dynamic_bitset<> const& previously_picked_rhs =
                picked_.try_emplace(lhs_sims, col_matches).first->second;
        boost::dynamic_bitset<> indices(col_matches);
        DecisionBoundaryVector const& rhs = *md.rhs_sims;
        for (model::Index i = 0; i < attribute_num_; ++i) {
            if (rhs[i] != 0.0) {
                indices.set(i);
            }
        }
        indices -= previously_picked_rhs;
        if (indices.none()) continue;
        min_picker_.AddGeneralizations(md, indices);
    }
    std::vector<ValidationInfo> collected = min_picker_.GetAll();
    if constexpr (MinPickerType::kNeedsEmptyRemoval) {
        util::EraseIfReplace(collected,
                             [](ValidationInfo const& info) { return info.rhs_indices.none(); });
    }
    for (ValidationInfo const& info : collected) {
        DecisionBoundaryVector const& lhs_sims = info.info->lhs_sims;
        std::size_t const col_matches = lhs_sims.size();
        boost::dynamic_bitset<>& validated_indices =
                picked_.try_emplace(lhs_sims, col_matches).first->second;
        assert((validated_indices & info.rhs_indices).none());
        validated_indices |= info.rhs_indices;
    }
    if (collected.empty()) {
        picked_.clear();
        ++cur_level_;
    }
    return collected;
}

}  // namespace algos::hymd::lattice::cardinality
