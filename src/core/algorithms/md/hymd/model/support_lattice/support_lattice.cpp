#include "support_lattice.h"

namespace algos::hymd::model {

bool SupportLattice::IsUnsupported(model::SimilarityVector const& lhs_vec) {
    return root_.IsUnsupported(lhs_vec, 0);
}

void SupportLattice::MarkUnsupported(model::SimilarityVector const& lhs_vec) {
    root_.MarkUnsupported(lhs_vec, 0);
}

}
