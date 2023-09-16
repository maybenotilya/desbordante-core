#pragma once

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

struct LatticeMd {
    SimilarityVector lhs_sims;
    Similarity rhs_sim;
    size_t rhs_index;

    LatticeMd(SimilarityVector lhs_sims, Similarity rhs_sim, size_t rhs_index)
        : lhs_sims(std::move(lhs_sims)), rhs_sim(rhs_sim), rhs_index(rhs_index) {}
};

}
