#pragma once

#include <vector>

namespace algos::hymd::model {

struct LatticeNode {
    std::vector<double> lhs_sims;
    std::vector<double> rhs_sims;

    LatticeNode(std::vector<double> lhs_sims, std::vector<double> rhs_sims)
        : lhs_sims(std::move(lhs_sims)), rhs_sims(std::move(rhs_sims)) {}
};

}
