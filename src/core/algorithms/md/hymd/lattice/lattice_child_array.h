#pragma once

#include <map>
#include <unordered_map>

#include "algorithms/md/decision_boundary.h"
#include "model/index.h"

namespace algos::hymd::lattice {
template <typename NodeType>
using BoundaryMap = std::map<model::md::DecisionBoundary, NodeType>;

template <typename NodeType>
using LatticeChildArray = std::unordered_map<model::Index, BoundaryMap<NodeType>>;
}  // namespace algos::hymd::lattice
