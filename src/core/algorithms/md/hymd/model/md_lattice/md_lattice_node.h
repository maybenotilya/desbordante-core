#pragma once

#include <cstddef>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/md_lattice/lattice_md.h"
#include "algorithms/md/hymd/model/md_lattice/lattice_node_sims.h"

namespace algos::hymd::model {

class MdLatticeNode {
private:
    using ThresholdMap = std::map<double, std::unique_ptr<MdLatticeNode>>;

    SimilarityVector rhs_;
    std::unordered_map<size_t, ThresholdMap> children_;

public:
    void GetMinimalOfCardinality(std::vector<LatticeNodeSims>& min_nodes, size_t sims_left,
                                 std::unordered_set<MdLatticeNode*> const& exclude);
    SimilarityVector GetMaxValidGeneralizationRhs(SimilarityVector const& lhs);

    void Add(LatticeMd const& md, size_t this_node_index);
    bool AddIfMin(LatticeMd const& md);
    bool HasGeneralization(LatticeMd const& md, size_t this_node_index);

    void FindViolated(std::vector<LatticeMd>& found, SimilarityVector& this_node_lhs,
                      SimilarityVector const& similarity_vector, size_t this_node_index);

    void RemoveMd(LatticeMd const& md, size_t this_node_index);
    void RemoveNode(SimilarityVector const& node, size_t this_node_index);

    explicit MdLatticeNode(size_t attributes_num) : rhs_(attributes_num) {}
    explicit MdLatticeNode(SimilarityVector rhs) : rhs_(std::move(rhs)) {}
};

}
