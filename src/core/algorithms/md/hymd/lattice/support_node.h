#pragma once

#include <cstddef>

#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/lattice/lattice_child_array.h"
#include "model/index.h"

namespace algos::hymd::lattice {

class SupportNode {
    bool is_unsupported_ = false;
    LatticeChildArray<SupportNode> children_;
    void MarkUnchecked(DecisionBoundaryVector const& lhs_bounds, model::Index this_node_index);

public:
    bool IsUnsupported(DecisionBoundaryVector const& lhs_bounds,
                       model::Index this_node_index) const;
    void MarkUnsupported(DecisionBoundaryVector const& lhs_bounds, model::Index this_node_index);

    SupportNode(std::size_t children_number);
};

}  // namespace algos::hymd::lattice
