#pragma once

#include <cstddef>
#include <functional>

#include "algorithms/md/decision_boundary.h"

namespace algos::hymd::lattice {
using SingleLevelFunc = std::function<std::size_t(model::md::DecisionBoundary, std::size_t)>;
}  // namespace algos::hymd::lattice
