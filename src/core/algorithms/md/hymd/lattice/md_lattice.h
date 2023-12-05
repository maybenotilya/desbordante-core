#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/lattice/md_lattice_node.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/single_level_func.h"
#include "algorithms/md/hymd/similarity_vector.h"
#include "model/index.h"

namespace algos::hymd::lattice {

class MdLattice {
private:
    size_t max_level_ = 0;
    MdLatticeNode root_;
    std::size_t const column_matches_size_;
    // Is there a way to define a level in such a way that one cannot use each decision boundary
    // independently to determine an MD's level and the lattice traversal algorithms works?
    SingleLevelFunc const get_single_level_;

public:
    std::size_t GetColMatchNumber() const {
        return column_matches_size_;
    }
    [[nodiscard]] bool HasGeneralization(DecisionBoundaryVector const& lhs_sims,
                                         model::md::DecisionBoundary rhs_sim,
                                         model::Index rhs_index) const;

    [[nodiscard]] size_t GetMaxLevel() const;
    std::vector<MdLatticeNodeInfo> GetLevel(size_t level);
    std::vector<model::md::DecisionBoundary> GetRhsInterestingnessBounds(
            DecisionBoundaryVector const& lhs, std::vector<model::Index> const& indices) const;
    void AddIfMinimal(DecisionBoundaryVector const& lhs_sims, model::md::DecisionBoundary rhs_sim,
                      model::Index rhs_index);
    std::vector<MdLatticeNodeInfo> FindViolated(SimilarityVector const& similarity_vector);

    explicit MdLattice(size_t column_matches_size, SingleLevelFunc single_level_func);
};

}  // namespace algos::hymd::lattice
