#pragma once

#include <map>
#include <unordered_map>

#include "algorithms/md/decision_boundary.h"
#include "model/index.h"

namespace algos::hymd::lattice {
template <typename NodeType>
using ThresholdMap = std::map<model::md::DecisionBoundary, NodeType>;

template <typename NodeType>
using LatticeChildArray = std::unordered_map<model::Index, ThresholdMap<NodeType>>;
}  // namespace algos::hymd::lattice