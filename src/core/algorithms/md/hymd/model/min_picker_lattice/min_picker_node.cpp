#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_node.h"

#include <cassert>

#include "algorithms/md/hymd/model/get_first_non_zero_index.h"

namespace algos::hymd::model {

void MinPickerNode::Add(LatticeNodeSims const& md, size_t this_node_index) {
    model::SimilarityVector const& lhs_vec = md.lhs_sims;
    assert(this_node_index < lhs_vec.size());
    size_t const first_non_zero_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (first_non_zero_index == lhs_vec.size()) {
        rhs_ = md.rhs_sims;
        return;
    }
    assert(first_non_zero_index < lhs_vec.size());
    size_t const child_array_index = first_non_zero_index - this_node_index;
    model::Similarity const child_similarity = lhs_vec[first_non_zero_index];
    ThresholdMap& threshold_map = children_[child_array_index];
    std::unique_ptr<MinPickerNode>& node_ptr = threshold_map[child_similarity];
    if (node_ptr == nullptr) {
        node_ptr = std::make_unique<MinPickerNode>();
    }
    node_ptr->Add(md, first_non_zero_index + 1);
}

bool MinPickerNode::HasGeneralization(LatticeNodeSims const& md, size_t this_node_index) {
    if (IsMd()) return true;
    SimilarityVector const& lhs_vec = md.lhs_sims;
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const next_node_index = this_node_index + 1 + index;
        assert(next_node_index < lhs_vec.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > lhs_vec[next_node_index]) {
                break;
            }
            if (node->HasGeneralization(md, next_node_index)) {
                return true;
            }
        }
    }
    return false;
}

void MinPickerNode::TryAdd(LatticeNodeSims const& md) {
    if (HasGeneralization(md, 0)) return;
    RemoveSpecializations(md, 0);
    Add(md, 0);
}

void MinPickerNode::RemoveSpecializations(LatticeNodeSims const& md, size_t this_node_index) {
    // All MDs in the tree are of the same cardinality.
    // if (IsMD()) { rhs_.clear(); return; }
    SimilarityVector const& lhs_vec = md.lhs_sims;
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const next_node_index = this_node_index + 1 + index;
        assert(next_node_index < lhs_vec.size());
        double const next_node_sim = lhs_vec[next_node_index];
        if (next_node_sim == 0.0) continue;
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold < next_node_sim) continue;
            RemoveSpecializations(md, next_node_index);
        }
    }
    rhs_.clear();
}

void MinPickerNode::GetAll(std::vector<LatticeNodeSims>& collected, SimilarityVector& this_node_lhs,
                           size_t this_node_index, size_t sims_left) {
    if (sims_left == 0) {
        if (IsMd()) collected.emplace_back(this_node_lhs, rhs_);
        return;
    }
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const next_node_index = this_node_index + 1 + index;
        assert(next_node_index < this_node_lhs.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            this_node_lhs[next_node_index] = threshold;
            node->GetAll(collected, this_node_lhs, this_node_index, sims_left - 1);
            this_node_lhs[next_node_index] = 0.0;
        }
    }
}

}  // namespace algos::hymd::model
