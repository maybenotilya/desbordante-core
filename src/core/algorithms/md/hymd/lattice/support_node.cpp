#include "algorithms/md/hymd/lattice/support_node.h"

#include <cassert>
#include <memory>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace algos::hymd::lattice {

void SupportNode::MarkUnchecked(DecisionBoundaryVector const& lhs_bounds,
                                model::Index this_node_index) {
    assert(children_.empty());
    size_t const col_match_number = lhs_bounds.size();
    SupportNode* cur_node_ptr = this;
    for (model::Index cur_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         cur_node_index != col_match_number;
         cur_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, cur_node_index + 1)) {
        cur_node_ptr = &cur_node_ptr->children_[cur_node_index]
                                .try_emplace(lhs_bounds[cur_node_index])
                                .first->second;
    }
    cur_node_ptr->is_unsupported_ = true;
}

bool SupportNode::IsUnsupported(DecisionBoundaryVector const& lhs_bounds) const {
    if (is_unsupported_) return true;
    for (auto const& [index, boundary_mapping] : children_) {
        model::md::DecisionBoundary const generalization_boundary_limit = lhs_bounds[index];
        for (auto const& [generalization_boundary, node] : boundary_mapping) {
            assert(generalization_boundary > 0.0);
            if (generalization_boundary > generalization_boundary_limit) break;
            if (node.IsUnsupported(lhs_bounds)) return true;
        }
    }
    return false;
}

void SupportNode::MarkUnsupported(DecisionBoundaryVector const& lhs_bounds,
                                  model::Index const this_node_index) {
    std::size_t const col_match_number = lhs_bounds.size();
    assert(this_node_index <= col_match_number);
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
    if (next_node_index == col_match_number) {
        is_unsupported_ = true;
        return;
    }
    assert(next_node_index < col_match_number);
    model::md::DecisionBoundary const child_bound = lhs_bounds[next_node_index];
    auto [it_arr, is_first] = children_.try_emplace(next_node_index);
    BoundaryMap<SupportNode>& boundary_map = it_arr->second;
    if (is_first) {
        boundary_map.try_emplace(child_bound)
                .first->second.MarkUnchecked(lhs_bounds, next_node_index + 1);
        return;
    }
    auto [it_map, is_first_map] = boundary_map.try_emplace(child_bound);
    SupportNode& next_node = it_map->second;
    if (is_first_map) {
        next_node.MarkUnchecked(lhs_bounds, next_node_index + 1);
        return;
    }
    next_node.MarkUnsupported(lhs_bounds, next_node_index + 1);
}

}  // namespace algos::hymd::lattice
