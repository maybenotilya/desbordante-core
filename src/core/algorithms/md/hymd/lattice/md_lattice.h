#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/lattice/md_lattice_node.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/similarity_vector.h"
#include "model/index.h"

namespace algos::hymd::lattice {

class MdLattice {
private:
    size_t max_level_ = 0;
    MdLatticeNode root_;
    size_t column_matches_size_;

public:
    [[nodiscard]] bool HasGeneralization(DecisionBoundaryVector const& lhs_sims,
                                         model::md::DecisionBoundary rhs_sim,
                                         model::Index rhs_index) const;

    [[nodiscard]] size_t GetMaxLevel() const;
    std::vector<MdLatticeNodeInfo> GetLevel(size_t level);
    std::vector<model::md::DecisionBoundary> GetMaxValidGeneralizationRhs(
            DecisionBoundaryVector const& lhs) const;

    void Add(DecisionBoundaryVector const& lhs_sims, model::md::DecisionBoundary rhs_sim,
             model::Index rhs_index);
    void AddIfMinimal(DecisionBoundaryVector const& lhs_sims, model::md::DecisionBoundary rhs_sim,
                      model::Index rhs_index);
    std::vector<MdLatticeNodeInfo> FindViolated(SimilarityVector const& similarity_vector);

    void RemoveNode(DecisionBoundaryVector const& lhs);

    explicit MdLattice(size_t column_matches_size);
};

}  // namespace algos::hymd::lattice
