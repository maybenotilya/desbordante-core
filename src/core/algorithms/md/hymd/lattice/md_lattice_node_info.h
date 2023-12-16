#pragma once

#include "algorithms/md/hymd/decision_boundary_vector.h"

namespace algos::hymd::lattice {

struct MdLatticeNodeInfo {
    DecisionBoundaryVector lhs_bounds;
    DecisionBoundaryVector* rhs_bounds;

    MdLatticeNodeInfo(DecisionBoundaryVector lhs_bounds, DecisionBoundaryVector* rhs_bounds)
        : lhs_bounds(std::move(lhs_bounds)), rhs_bounds(rhs_bounds) {}
};

}  // namespace algos::hymd::lattice
