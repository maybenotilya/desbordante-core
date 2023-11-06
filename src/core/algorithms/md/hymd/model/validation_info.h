#pragma once

#include <cstddef>
#include <unordered_set>

#include "algorithms/md/hymd/model/md_lattice/md_lattice_node_info.h"

namespace algos::hymd::model {

struct ValidationInfo {
    MdLatticeNodeInfo* info;
    std::unordered_set<size_t> rhs_indices;
};

}  // namespace algos::hymd::model
