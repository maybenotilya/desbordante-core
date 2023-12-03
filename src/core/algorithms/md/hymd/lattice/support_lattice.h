#pragma once

#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/lattice/support_node.h"

namespace algos::hymd::lattice {

class SupportLattice {
private:
    SupportNode root_;

public:
    void MarkUnsupported(DecisionBoundaryVector const& lhs_vec);
    bool IsUnsupported(DecisionBoundaryVector const& lhs_vec);
};

}  // namespace algos::hymd::lattice