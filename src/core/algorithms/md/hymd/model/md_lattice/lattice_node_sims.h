#pragma once

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

struct LatticeNodeSims {
    SimilarityVector lhs_sims;
    SimilarityVector rhs_sims;

    LatticeNodeSims(SimilarityVector lhs_sims, SimilarityVector rhs_sims)
        : lhs_sims(std::move(lhs_sims)), rhs_sims(std::move(rhs_sims)) {}
};

}  // namespace algos::hymd::model
