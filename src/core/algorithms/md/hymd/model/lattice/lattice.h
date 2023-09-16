#pragma once

#include <cstddef>

#include "algorithms/md/hymd/model/lattice/lattice_md.h"
#include "algorithms/md/hymd/model/lattice/lattice_node.h"
#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class Lattice {
private:
    // ...

public:
    size_t GetMaxLevel();
    std::vector<LatticeNode> GetLevel(size_t level);
    model::SimilarityVector GetMaxValidGeneralizationRhs(model::SimilarityVector const& lhs);

    void Add(LatticeMd md);
    void AddIfMin(LatticeMd md);
    void FindViolated();

    void RemoveMd(LatticeMd md);
    void RemoveNode(LatticeNode node);

    Lattice(size_t column_matches_size);
};

}  // namespace algos::hymd::model
