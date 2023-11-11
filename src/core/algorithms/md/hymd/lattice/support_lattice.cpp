#include "algorithms/md/hymd/lattice/support_lattice.h"

namespace algos::hymd::lattice {

bool SupportLattice::IsUnsupported(DecisionBoundaryVector const& lhs_vec) {
    return root_.IsUnsupported(lhs_vec, 0);
}

void SupportLattice::MarkUnsupported(DecisionBoundaryVector const& lhs_vec) {
    root_.MarkUnsupported(lhs_vec, 0);
}

}  // namespace algos::hymd::lattice
