#include "algorithms/md/hymd/lattice/cardinality/min_picker_node.h"

#include <algorithm>
#include <cassert>
#include <optional>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace {
using BoundMap =
        algos::hymd::lattice::BoundaryMap<algos::hymd::lattice::cardinality::MinPickerNode>;
using OptionalChild = std::optional<BoundMap>;
}  // namespace

namespace algos::hymd::lattice::cardinality {

void MinPickerNode::AddUnchecked(ValidationInfo* validation_info, model::Index this_node_index) {
    assert(children_.empty());
    DecisionBoundaryVector const& lhs_bounds = validation_info->node_info->lhs_bounds;
    size_t const col_match_number = lhs_bounds.size();
    MinPickerNode* cur_node_ptr = this;
    for (model::Index next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         next_node_index != col_match_number;
         next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, next_node_index + 1)) {
        std::size_t const child_array_index = next_node_index - this_node_index;
        std::size_t const next_child_array_size = col_match_number - next_node_index;
        cur_node_ptr = &cur_node_ptr->children_[child_array_index]
                                .emplace()
                                .try_emplace(lhs_bounds[next_node_index], next_child_array_size)
                                .first->second;
        this_node_index = next_node_index + 1;
    }
    cur_node_ptr->task_info_ = validation_info;
}

void MinPickerNode::Add(ValidationInfo* validation_info, model::Index this_node_index) {
    DecisionBoundaryVector const& lhs_bounds = validation_info->node_info->lhs_bounds;
    assert(this_node_index <= lhs_bounds.size());
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
    std::size_t const col_match_number = lhs_bounds.size();
    if (next_node_index == col_match_number) {
        assert(task_info_ == nullptr);
        task_info_ = validation_info;
        return;
    }
    assert(next_node_index < col_match_number);
    model::md::DecisionBoundary const next_lhs_bound = lhs_bounds[next_node_index];
    model::Index const child_array_index = next_node_index - this_node_index;
    std::size_t const next_child_array_size = col_match_number - next_node_index;
    auto [threshold_map, is_first_arr] = TryEmplaceChild(children_, child_array_index);
    if (is_first_arr) {
        threshold_map.try_emplace(next_lhs_bound, next_child_array_size)
                .first->second.AddUnchecked(validation_info, next_node_index + 1);
        return;
    }
    auto [it_map, is_first_map] = threshold_map.try_emplace(next_lhs_bound, next_child_array_size);
    MinPickerNode& next_node = it_map->second;
    if (is_first_map) {
        next_node.AddUnchecked(validation_info, next_node_index + 1);
        return;
    }
    next_node.Add(validation_info, next_node_index + 1);
}

void MinPickerNode::ExcludeGeneralizationRhs(MdLatticeNodeInfo const& lattice_node_info,
                                             model::Index this_node_index,
                                             boost::dynamic_bitset<>& considered_indices) {
    if (task_info_ != nullptr) {
        boost::dynamic_bitset<> const& this_node_indices = task_info_->rhs_indices;
        considered_indices -= this_node_indices;
        if (considered_indices.none()) return;
    }
    DecisionBoundaryVector const& lhs_bounds = lattice_node_info.lhs_bounds;
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
    model::Index const child_array_index = next_node_index - this_node_index;
    OptionalChild& optional_child = children_[child_array_index];
    if (!optional_child.has_value()) return;
    assert(next_node_index < lhs_bounds.size());
    BoundaryMap<MinPickerNode>& threshold_mapping = *optional_child;
    model::md::DecisionBoundary const next_lhs_bound = lhs_bounds[next_node_index];
    for (auto& [threshold, node] : threshold_mapping) {
        assert(threshold > 0.0);
        if (threshold > next_lhs_bound) break;
        node.ExcludeGeneralizationRhs(lattice_node_info, next_node_index + 1, considered_indices);
        if (considered_indices.none()) return;
    }
}

void MinPickerNode::RemoveSpecializations(MdLatticeNodeInfo const& lattice_node_info,
                                          model::Index this_node_index,
                                          boost::dynamic_bitset<> const& picked_indices) {
    // All MDs in the tree are of the same cardinality.
    DecisionBoundaryVector const& lhs_bounds = lattice_node_info.lhs_bounds;
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
    if (next_node_index == lhs_bounds.size()) {
        assert(children_.empty());
        if (task_info_ != nullptr) {
            boost::dynamic_bitset<>& this_node_rhs_indices = task_info_->rhs_indices;
            this_node_rhs_indices -= picked_indices;
            if (this_node_rhs_indices.none()) task_info_ = nullptr;
        }
        return;
    }
    model::Index const child_array_index = next_node_index - this_node_index;
    OptionalChild& optional_child = children_[child_array_index];
    if (!optional_child.has_value()) return;
    model::md::DecisionBoundary const next_node_bound = lhs_bounds[next_node_index];
    BoundaryMap<MinPickerNode>& threshold_mapping = *optional_child;
    auto mapping_end = threshold_mapping.end();
    for (auto it_map = threshold_mapping.lower_bound(next_node_bound); it_map != mapping_end;
         ++it_map) {
        auto& node = it_map->second;
        node.RemoveSpecializations(lattice_node_info, next_node_index + 1, picked_indices);
    }
}

void MinPickerNode::GetAll(std::vector<ValidationInfo>& collected, model::Index this_node_index) {
    if (task_info_ != nullptr) {
        collected.push_back(std::move(*task_info_));
    }
    std::size_t const child_array_size = children_.size();
    for (model::Index child_array_index = FindFirstNonEmptyIndex(children_, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(children_, child_array_index + 1)) {
        model::Index const next_node_index = this_node_index + child_array_index;
        for (auto& [boundary, node] : *children_[child_array_index]) {
            node.GetAll(collected, next_node_index + 1);
        }
    }
}

void MinPickerNode::Clear() {
    task_info_ = nullptr;
    children_.assign(children_.size(), std::nullopt);
}

MinPickerNode::MinPickerNode(std::size_t children_number) : children_(children_number) {}

}  // namespace algos::hymd::lattice::cardinality
