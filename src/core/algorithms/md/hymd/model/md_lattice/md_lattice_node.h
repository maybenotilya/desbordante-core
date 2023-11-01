#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/model/md_lattice/lattice_md.h"
#include "algorithms/md/hymd/model/md_lattice/lattice_node_sims.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class MdLatticeNode {
private:
    using ThresholdMap = std::map<double, std::unique_ptr<MdLatticeNode>>;
    using ChildArray = std::unordered_map<size_t, ThresholdMap>;

    SimilarityVector rhs_;
    ChildArray children_;

public:
    void GetLevel(std::vector<LatticeNodeSims>& collected, SimilarityVector& this_node_lhs,
                  size_t this_node_index, size_t sims_left) const;
    void GetMaxValidGeneralizationRhs(SimilarityVector const& lhs, SimilarityVector& cur_rhs,
                                      size_t this_node_index) const;

    void Add(SimilarityVector const& lhs_sims, Similarity rhs_sim, size_t rhs_index,
             size_t this_node_index);
    [[nodiscard]] bool HasGeneralization(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                         size_t rhs_index, size_t this_node_index) const;

    void FindViolated(std::vector<MdLatticeNodeInfo>& found, SimilarityVector& this_node_lhs,
                      SimilarityVector const& similarity_vector, size_t this_node_index);

    void RemoveNode(SimilarityVector const& node, size_t this_node_index);

    explicit MdLatticeNode(size_t attributes_num) : rhs_(attributes_num) {}
    explicit MdLatticeNode(SimilarityVector rhs) : rhs_(std::move(rhs)) {}
};

}  // namespace algos::hymd::model
