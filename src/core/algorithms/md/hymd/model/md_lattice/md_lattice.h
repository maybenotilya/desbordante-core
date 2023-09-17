#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "algorithms/md/hymd/model/md_lattice/lattice_md.h"
#include "algorithms/md/hymd/model/md_lattice/lattice_node_sims.h"
#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class MdLattice {
private:
    size_t max_level_ = 0;

public:


    size_t GetMaxLevel();
    //std::vector<LatticeNode> GetLevel(size_t level);
    std::vector<LatticeNodeSims> GetMinimalOfCardinality(size_t cardinality);
    model::SimilarityVector GetMaxValidGeneralizationRhs(model::SimilarityVector const& lhs);

    void Add(LatticeMd md);
    void AddIfMin(LatticeMd md);
    std::vector<LatticeMd> FindViolated(model::SimilarityVector similarity_vector);

    void RemoveMd(LatticeMd md);
    void RemoveNode(LatticeNodeSims node);

    MdLattice(size_t column_matches_size);
    // Needs cardinality
    // AKA `Validate`
    std::pair<std::vector<double>, size_t> GetMaxRhsDecBounds(
            model::SimilarityVector const& lhs_sims);
};

}  // namespace algos::hymd::model
