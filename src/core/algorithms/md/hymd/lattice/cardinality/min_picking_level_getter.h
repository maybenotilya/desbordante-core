#pragma once

#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"
#include "algorithms/md/hymd/lattice/cardinality/one_by_one_min_picker.h"
#include "algorithms/md/hymd/lattice/level_getter.h"

namespace algos::hymd::lattice::cardinality {

class MinPickingLevelGetter final : public LevelGetter {
private:
    size_t const attribute_num_;
    OneByOnePicker min_picker_;
    std::unordered_map<DecisionBoundaryVector, std::unordered_set<model::Index>> picked_;

    std::vector<ValidationInfo> GetCurrentMdsInternal(
            std::vector<lattice::MdLatticeNodeInfo>& level_mds) final;

public:
    MinPickingLevelGetter(FullLattice* lattice)
        : LevelGetter(lattice),
          attribute_num_(lattice_->GetColMatchNumber()),
          min_picker_() {}
};

}  // namespace algos::hymd::lattice::cardinality
