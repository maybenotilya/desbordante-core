#pragma once

#include <cstddef>

#include "algorithms/md/hymd/model/md_lattice/lattice_node_sims.h"

namespace algos::hymd::model {

class MinPickerLattice {
private:


public:
    std::vector<LatticeNodeSims> PickMinimumMDs(std::vector<LatticeNodeSims> const& mds);

    void Reset();
};

}