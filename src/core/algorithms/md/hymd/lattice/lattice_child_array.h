#pragma once

#include <map>
#include <memory>
#include <unordered_map>

#include "algorithms/md/decision_boundary.h"
#include "model/index.h"

namespace algos::hymd::lattice {
template <typename NodeType>
using ThresholdMap = std::map<model::md::DecisionBoundary, std::unique_ptr<NodeType>>;

template <typename NodeType>
using LatticeChildArray = std::unordered_map<model::Index, ThresholdMap<NodeType>>;
}  // namespace algos::hymd::lattice
