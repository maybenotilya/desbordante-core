#pragma once

#include <vector>

namespace algos::hymd::model {

struct LatticeNodeSims {
    std::vector<double> lhs_sims;
    std::vector<double> rhs_sims;

    LatticeNodeSims(std::vector<double> lhs_sims, std::vector<double> rhs_sims)
        : lhs_sims(std::move(lhs_sims)), rhs_sims(std::move(rhs_sims)) {}
};

}
