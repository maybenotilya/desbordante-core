#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_node.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/hymd/model/get_first_non_zero_index.h"

namespace algos::hymd::model {

void MinPickerNode::Add(MdLatticeNodeInfo& md, size_t this_node_index,
                        std::unordered_set<size_t>& indices) {
    model::SimilarityVector const& lhs_vec = md.lhs_sims;
    assert(this_node_index <= lhs_vec.size());
    size_t const first_non_zero_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (first_non_zero_index == lhs_vec.size()) {
        assert(!task_info_);
        task_info_ = {&md, std::move(indices)};
        return;
    }
    assert(first_non_zero_index < lhs_vec.size());
    model::Similarity const child_similarity = lhs_vec[first_non_zero_index];
    ThresholdMap& threshold_map = children_[first_non_zero_index];
    std::unique_ptr<MinPickerNode>& node_ptr = threshold_map[child_similarity];
    if (node_ptr == nullptr) {
        node_ptr = std::make_unique<MinPickerNode>();
    }
    node_ptr->Add(md, first_non_zero_index + 1, indices);
}

void MinPickerNode::ExcludeGeneralizationRhs(MdLatticeNodeInfo const& md, size_t this_node_index,
                                             std::unordered_set<size_t>& considered_indices) {
    if (task_info_.has_value()) {
        ValidationInfo& value = task_info_.value();
        std::unordered_set<size_t> const& indices = value.rhs_indices;
        for (auto it = considered_indices.begin(); it != considered_indices.end();) {
            size_t const index = *it;
            if (indices.find(index) != indices.end()) {
                it = considered_indices.erase(it);
            } else {
                ++it;
            }
        }
        if (considered_indices.empty()) return;
    }
    SimilarityVector const& lhs_vec = md.lhs_sims;
    size_t const next_node_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    auto it = children_.find(next_node_index);
    if (it == children_.end()) return;
    assert(next_node_index < lhs_vec.size());
    ThresholdMap& threshold_mapping = it->second;
    Similarity const next_lhs_sim = lhs_vec[next_node_index];
    for (auto const& [threshold, node] : threshold_mapping) {
        assert(threshold > 0.0);
        if (threshold > next_lhs_sim) break;
        node->ExcludeGeneralizationRhs(md, next_node_index + 1, considered_indices);
        if (considered_indices.empty()) return;
    }
}

void MinPickerNode::RemoveSpecializations(MdLatticeNodeInfo const& md, size_t this_node_index,
                                          std::unordered_set<size_t> const& indices) {
    // All MDs in the tree are of the same cardinality.
    SimilarityVector const& lhs_vec = md.lhs_sims;
    size_t const next_node_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (next_node_index == lhs_vec.size()) {
        if (task_info_) {
            std::unordered_set<size_t>& rhs_indices = task_info_->rhs_indices;
            std::for_each(indices.begin(), indices.end(),
                          [&rhs_indices](size_t index) { rhs_indices.erase(index); });
            if (rhs_indices.empty()) task_info_ = std::nullopt;
        }
        return;
    }
    auto it = children_.find(next_node_index);
    if (it == children_.end()) return;
    Similarity const next_node_sim = lhs_vec[next_node_index];
    ThresholdMap& threshold_mapping = it->second;
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

}  // namespace algos::hymd::model
