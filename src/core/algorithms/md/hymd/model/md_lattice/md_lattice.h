#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "algorithms/md/hymd/model/md_lattice/lattice_md.h"
#include "algorithms/md/hymd/model/md_lattice/lattice_node_sims.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice_node.h"
#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class MdLattice {
private:
    size_t max_level_ = 0;
    MdLatticeNode root_;

public:
    [[nodiscard]] size_t GetMaxLevel() const;
    std::pair<std::vector<LatticeNodeSims>, std::unordered_set<MdLatticeNode*>> GetLevel(
            size_t level);
    SimilarityVector GetMaxValidGeneralizationRhs(SimilarityVector const& lhs);

    void Add(LatticeMd const& md);
    void AddIfMin(LatticeMd const& md);
    std::vector<LatticeMd> FindViolated(SimilarityVector const& similarity_vector);

    void RemoveMd(LatticeMd const& md);
    void RemoveNode(SimilarityVector const& lhs);

    MdLattice(size_t column_matches_size);
};

}  // namespace algos::hymd::model
