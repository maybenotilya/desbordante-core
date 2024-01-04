#include "algorithms/md/hymd/lattice/support_node.h"

#include <cassert>
#include <memory>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace {
using BoundMap = algos::hymd::lattice::BoundaryMap<algos::hymd::lattice::SupportNode>;
}  // namespace

namespace algos::hymd::lattice {

void SupportNode::MarkUnchecked(DecisionBoundaryVector const& lhs_bounds,
                                model::Index this_node_index) {
    assert(IsEmpty(children_));
    size_t const col_match_number = lhs_bounds.size();
    SupportNode* cur_node_ptr = this;
    for (model::Index next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         next_node_index != col_match_number;
         next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index)) {
        std::size_t const child_array_index = next_node_index - this_node_index;
        std::size_t const next_child_array_size = col_match_number - next_node_index;
        cur_node_ptr = &cur_node_ptr->children_[child_array_index]
                                .emplace()
                                .try_emplace(lhs_bounds[next_node_index], next_child_array_size)
                                .first->second;
        this_node_index = next_node_index + 1;
    }
    cur_node_ptr->is_unsupported_ = true;
}

bool SupportNode::IsUnsupported(DecisionBoundaryVector const& lhs_bounds,
                                model::Index this_node_index) const {
    if (is_unsupported_) return true;
    std::size_t const child_array_size = children_.size();
    for (model::Index child_array_index = FindFirstNonEmptyIndex(children_, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(children_, child_array_index + 1)) {
        model::Index const next_node_index = this_node_index + child_array_index;
        model::md::DecisionBoundary const generalization_boundary_limit =
                lhs_bounds[next_node_index];
        for (auto const& [generalization_boundary, node] : *children_[child_array_index]) {
            if (generalization_boundary > generalization_boundary_limit) break;
            if (node.IsUnsupported(lhs_bounds, next_node_index + 1)) return true;
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
    model::md::DecisionBoundary const next_bound = lhs_bounds[next_node_index];
    model::Index const child_array_index = next_node_index - this_node_index;
    std::size_t const next_child_array_size = col_match_number - next_node_index;
    auto [boundary_map, is_first_arr] = TryEmplaceChild(children_, child_array_index);
    if (is_first_arr) {
        boundary_map.try_emplace(next_bound, next_child_array_size)
                .first->second.MarkUnchecked(lhs_bounds, next_node_index + 1);
        return;
    }
    auto [it_map, is_first_map] = boundary_map.try_emplace(next_bound, next_child_array_size);
    SupportNode& next_node = it_map->second;
    if (is_first_map) {
        next_node.MarkUnchecked(lhs_bounds, next_node_index + 1);
        return;
    }
    next_node.MarkUnsupported(lhs_bounds, next_node_index + 1);
}

SupportNode::SupportNode(std::size_t children_number) : children_(children_number) {}

}  // namespace algos::hymd::lattice
