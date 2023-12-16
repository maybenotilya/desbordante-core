#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/lattice/lattice_child_array.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/single_level_func.h"
#include "algorithms/md/hymd/similarity_vector.h"
#include "model/index.h"

namespace algos::hymd::lattice {

class MdLatticeNode {
private:
    DecisionBoundaryVector rhs_bounds_;
    LatticeChildArray<MdLatticeNode> children_;

    void AddUnchecked(DecisionBoundaryVector const& lhs_bounds,
                      model::md::DecisionBoundary rhs_bound, model::Index rhs_index,
                      model::Index this_node_index);

public:
    void GetLevel(std::vector<MdLatticeNodeInfo>& collected,
                  DecisionBoundaryVector& this_node_lhs_bounds, model::Index this_node_index,
                  std::size_t level_left, SingleLevelFunc const& single_level_func);
    void RaiseInterestingnessBounds(
            DecisionBoundaryVector const& lhs_bounds,
            std::vector<model::md::DecisionBoundary>& cur_interestingness_bounds,
            model::Index this_node_index, std::vector<model::Index> const& indices) const;

    [[nodiscard]] bool HasGeneralization(DecisionBoundaryVector const& lhs_bounds,
                                         model::md::DecisionBoundary rhs_bound,
                                         model::Index rhs_index,
                                         model::Index this_node_index) const;
    bool AddIfMinimal(DecisionBoundaryVector const& lhs_bounds,
                      model::md::DecisionBoundary rhs_bound, model::Index rhs_index,
                      model::Index this_node_index);

    void FindViolated(std::vector<MdLatticeNodeInfo>& found,
                      DecisionBoundaryVector& this_node_lhs_bounds,
                      SimilarityVector const& similarity_vector, model::Index this_node_index);

    explicit MdLatticeNode(std::size_t attributes_num) : rhs_bounds_(attributes_num) {}
    explicit MdLatticeNode(DecisionBoundaryVector rhs) : rhs_bounds_(std::move(rhs)) {}
};

}  // namespace algos::hymd::lattice
