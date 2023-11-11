#include "algorithms/md/hymd/lattice/cardinality/min_picker_node.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace algos::hymd::lattice::cardinality {

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
    ThresholdMap<MinPickerNode>& threshold_map = children_[first_non_zero_index];
    std::unique_ptr<MinPickerNode>& node_ptr = threshold_map[child_similarity];
    if (node_ptr == nullptr) {
        node_ptr = std::make_unique<MinPickerNode>();
    }
    node_ptr->Add(md, first_non_zero_index + 1, indices);
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
    for (auto const& [threshold, node] : threshold_mapping) {
        assert(threshold > 0.0);
        if (threshold > next_lhs_sim) break;
        node->ExcludeGeneralizationRhs(md, next_node_index + 1, considered_indices);
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
        auto const& node = it_map->second;
        node->RemoveSpecializations(md, next_node_index + 1, indices);
    }
}

void MinPickerNode::GetAll(std::vector<ValidationInfo*>& collected, size_t sims_left) {
    if (sims_left == 0) {
        if (task_info_.has_value()) {
            assert(!task_info_->rhs_indices.empty());
            collected.push_back(std::addressof(task_info_.value()));
        }
        return;
    }
    for (auto const& [index, threshold_mapping] : children_) {
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            node->GetAll(collected, sims_left - 1);
        }
    }
}

}  // namespace algos::hymd::lattice::cardinality
