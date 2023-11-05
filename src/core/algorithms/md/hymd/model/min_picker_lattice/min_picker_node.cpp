#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_node.h"

#include <cassert>

#include "algorithms/md/hymd/model/get_first_non_zero_index.h"

namespace algos::hymd::model {

void MinPickerNode::Add(MdLatticeNodeInfo const& md, size_t this_node_index) {
    model::SimilarityVector const& lhs_vec = md.lhs_sims;
    assert(this_node_index <= lhs_vec.size());
    size_t const first_non_zero_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (first_non_zero_index == lhs_vec.size()) {
        rhs_ = md.rhs_sims;
        return;
    }
    assert(first_non_zero_index < lhs_vec.size());
    model::Similarity const child_similarity = lhs_vec[first_non_zero_index];
    ThresholdMap& threshold_map = children_[first_non_zero_index];
    std::unique_ptr<MinPickerNode>& node_ptr = threshold_map[child_similarity];
    if (node_ptr == nullptr) {
        node_ptr = std::make_unique<MinPickerNode>();
    }
    node_ptr->Add(md, first_non_zero_index + 1);
}

bool MinPickerNode::HasGeneralization(MdLatticeNodeInfo const& md, size_t this_node_index) {
    if (IsMd()) return true;
    SimilarityVector const& lhs_vec = md.lhs_sims;
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const cur_node_index = index;
        assert(cur_node_index < lhs_vec.size());
        Similarity const cur_lhs_sim = lhs_vec[index];
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > cur_lhs_sim) break;
            if (node->HasGeneralization(md, cur_node_index + 1)) return true;
        }
    }
    return false;
}

void MinPickerNode::TryAdd(MdLatticeNodeInfo const& md) {
    if (HasGeneralization(md, 0)) return;
    RemoveSpecializations(md, 0);
    Add(md, 0);
}

void MinPickerNode::RemoveSpecializations(MdLatticeNodeInfo const& md, size_t this_node_index) {
    // All MDs in the tree are of the same cardinality.
    // if (IsMD()) { rhs_.clear(); return; }
    SimilarityVector const& lhs_vec = md.lhs_sims;
    for (auto const& [index, threshold_mapping] : children_) {
        assert(cur_node_index < lhs_vec.size());
        double const next_node_sim = lhs_vec[index];
        if (next_node_sim == 0.0) continue;
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold < next_node_sim) continue;
            node->RemoveSpecializations(md, index + 1);
        }
    }
    rhs_ = nullptr;
}

void MinPickerNode::GetAll(std::vector<MdLatticeNodeInfo>& collected,
                           SimilarityVector& this_node_lhs, size_t this_node_index,
                           size_t sims_left) {
    if (sims_left == 0) {
        if (IsMd()) collected.emplace_back(this_node_lhs, rhs_);
        return;
    }
    for (auto const& [index, threshold_mapping] : children_) {
        assert(cur_node_index < this_node_lhs.size());
        Similarity& cur_lhs_sim = this_node_lhs[index];
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            cur_lhs_sim = threshold;
            node->GetAll(collected, this_node_lhs, index + 1, sims_left - 1);
        }
        cur_lhs_sim = 0.0;
    }
}

}  // namespace algos::hymd::model
