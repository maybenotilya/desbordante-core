#include "algorithms/md/hymd/lattice/support_lattice.h"

namespace algos::hymd::lattice {

bool SupportLattice::IsUnsupported(DecisionBoundaryVector const& lhs_bounds) {
    return root_.IsUnsupported(lhs_bounds);
}

void SupportLattice::MarkUnsupported(DecisionBoundaryVector const& lhs_bounds) {
    root_.MarkUnsupported(lhs_bounds, 0);
}

}  // namespace algos::hymd::lattice
