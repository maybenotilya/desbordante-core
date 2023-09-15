#pragma once

#include <cstddef>

#include "algorithms/md/hymd/model/lattice/lattice_node.h"
#include "algorithms/md/hymd/model/lattice/lattice_md.h"

namespace algos::hymd::model {

class Lattice {
public:
    size_t GetMaxLevel();
    std::vector<LatticeNode> GetLevel(size_t level);
    std::vector<double> GetLowerBoundaries();

    void Add(LatticeMd md);
    void AddIfMin(LatticeMd md);
    void FindViolated();

    void RemoveMd(LatticeMd md);
    void RemoveNode(LatticeNode node);
};

}  // namespace algos::hymd::model
