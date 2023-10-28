#pragma once

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

struct MdLatticeNodeInfo {
    SimilarityVector const lhs_sims;
    SimilarityVector* const rhs_sims;

    MdLatticeNodeInfo(SimilarityVector lhs_sims, SimilarityVector* rhs_sims)
        : lhs_sims(std::move(lhs_sims)), rhs_sims(rhs_sims) {}
};

}  // namespace algos::hymd::model
