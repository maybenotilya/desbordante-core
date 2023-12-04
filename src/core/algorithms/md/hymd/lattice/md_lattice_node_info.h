#pragma once

#include "algorithms/md/hymd/decision_boundary_vector.h"

namespace algos::hymd::lattice {

struct MdLatticeNodeInfo {
    DecisionBoundaryVector lhs_sims;
    DecisionBoundaryVector* rhs_sims;

    MdLatticeNodeInfo(DecisionBoundaryVector lhs_sims, DecisionBoundaryVector* rhs_sims)
        : lhs_sims(std::move(lhs_sims)), rhs_sims(rhs_sims) {}
};

}  // namespace algos::hymd::lattice
