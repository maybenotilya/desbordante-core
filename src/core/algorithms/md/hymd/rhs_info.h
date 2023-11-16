#pragma once

#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/recommendation.h"
#include "model/index.h"

namespace algos::hymd {
struct RhsInfo {
    model::Index const index;
    model::md::DecisionBoundary const old_decision_boundary;
    std::vector<Recommendation> violations;

    RhsInfo(model::Index index, model::md::DecisionBoundary old_decision_boundary)
        : index(index), old_decision_boundary(old_decision_boundary) {}
};
}  // namespace algos::hymd
