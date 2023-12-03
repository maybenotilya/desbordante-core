#include "algorithms/md/hymd/lattice/cardinality/min_picker_node.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace algos::hymd::lattice::cardinality {

void MinPickerNode::AddUnchecked(MdLatticeNodeInfo& md, model::Index this_node_index,
                        std::unordered_set<model::Index>& indices) {
    assert(children_.empty());
    DecisionBoundaryVector const& lhs_vec = md.lhs_sims;
    size_t const size = lhs_vec.size();
    MinPickerNode* cur_node_ptr = this;
    for (model::Index cur_node_index = utility::GetFirstNonZeroIndex(lhs_vec, this_node_index);
         cur_node_index != size;
         cur_node_index = utility::GetFirstNonZeroIndex(lhs_vec, cur_node_index + 1)) {
        cur_node_ptr = &cur_node_ptr->children_.try_emplace(cur_node_index)
                                .first->second.try_emplace(lhs_vec[cur_node_index])
                                .first->second;
    }
    cur_node_ptr->task_info_ = {&md, std::move(indices)};
}

void MinPickerNode::Add(MdLatticeNodeInfo& md, model::Index this_node_index,
                        std::unordered_set<model::Index>& indices) {
    DecisionBoundaryVector const& lhs_vec = md.lhs_sims;
    assert(this_node_index <= lhs_vec.size());
    model::Index const first_non_zero_index =
            utility::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (first_non_zero_index == lhs_vec.size()) {
        assert(!task_info_);
        task_info_ = {&md, std::move(indices)};
        return;
    }
    assert(first_non_zero_index < lhs_vec.size());
    model::md::DecisionBoundary const child_similarity = lhs_vec[first_non_zero_index];
    auto [it_arr, is_first_arr] = children_.try_emplace(first_non_zero_index);
    ThresholdMap<MinPickerNode>& threshold_map = it_arr->second;
    if (is_first_arr) {
        threshold_map.try_emplace(child_similarity)
                .first->second.AddUnchecked(md, first_non_zero_index + 1, indices);
        return;
    }
    auto [it_map, is_first_map] = threshold_map.try_emplace(child_similarity);
    MinPickerNode& next_node = it_map->second;
    if (is_first_map) {
        next_node.AddUnchecked(md, first_non_zero_index + 1, indices);
        return;
    }
    next_node.Add(md, first_non_zero_index + 1, indices);
}

void MinPickerNode::ExcludeGeneralizationRhs(MdLatticeNodeInfo const& md,
                                             model::Index this_node_index,
                                             std::unordered_set<model::Index>& considered_indices) {
    if (task_info_.has_value()) {
        ValidationInfo& value = task_info_.value();
        std::unordered_set<model::Index> const& indices = value.rhs_indices;
        for (auto it = considered_indices.begin(); it != considered_indices.end();) {
            model::Index const index = *it;
            if (indices.find(index) != indices.end()) {
                it = considered_indices.erase(it);
            } else {
                ++it;
            }
        }
        if (considered_indices.empty()) return;
    }
    DecisionBoundaryVector const& lhs_vec = md.lhs_sims;
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    auto it = children_.find(next_node_index);
    if (it == children_.end()) return;
    assert(next_node_index < lhs_vec.size());
    ThresholdMap<MinPickerNode>& threshold_mapping = it->second;
    model::md::DecisionBoundary const next_lhs_sim = lhs_vec[next_node_index];
    for (auto& [threshold, node] : threshold_mapping) {
        assert(threshold > 0.0);
        if (threshold > next_lhs_sim) break;
        node.ExcludeGeneralizationRhs(md, next_node_index + 1, considered_indices);
        if (considered_indices.empty()) return;
    }
}

void MinPickerNode::RemoveSpecializations(MdLatticeNodeInfo const& md, model::Index this_node_index,
                                          std::unordered_set<model::Index> const& indices) {
    // All MDs in the tree are of the same cardinality.
    DecisionBoundaryVector const& lhs_vec = md.lhs_sims;
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (next_node_index == lhs_vec.size()) {
        assert(children_.empty());
        if (task_info_) {
            std::unordered_set<model::Index>& rhs_indices = task_info_->rhs_indices;
            std::for_each(indices.begin(), indices.end(),
                          [&rhs_indices](model::Index index) { rhs_indices.erase(index); });
            if (rhs_indices.empty()) task_info_ = std::nullopt;
        }
        return;
    }
    auto it = children_.find(next_node_index);
    if (it == children_.end()) return;
    model::md::DecisionBoundary const next_node_sim = lhs_vec[next_node_index];
    ThresholdMap<MinPickerNode>& threshold_mapping = it->second;
    for (auto it_map = threshold_mapping.lower_bound(next_node_sim);
         it_map != threshold_mapping.end(); ++it_map) {
        auto& node = it_map->second;
        node.RemoveSpecializations(md, next_node_index + 1, indices);
    }
}

void MinPickerNode::GetAll(std::vector<ValidationInfo*>& collected) {
    if (task_info_.has_value()) {
        assert(!task_info_->rhs_indices.empty());
        collected.push_back(std::addressof(*task_info_));
    }
    for (auto& [index, threshold_mapping] : children_) {
        for (auto& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            node.GetAll(collected);
        }
    }
}

}  // namespace algos::hymd::lattice::cardinality
