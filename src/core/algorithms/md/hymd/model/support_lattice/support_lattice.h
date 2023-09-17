#pragma once

#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/support_lattice/support_node.h"

namespace algos::hymd::model {

class SupportLattice {
private:
    SupportNode root_;

public:
    void MarkUnsupported(model::SimilarityVector const& lhs_vec);
    bool IsUnsupported(model::SimilarityVector const& lhs_vec);
};

}