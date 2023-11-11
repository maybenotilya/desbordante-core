#pragma once

#include <unordered_set>

#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "model/index.h"

namespace algos::hymd::lattice {

struct ValidationInfo {
    MdLatticeNodeInfo* info;
    std::unordered_set<model::Index> rhs_indices;
};

}  // namespace algos::hymd::lattice
