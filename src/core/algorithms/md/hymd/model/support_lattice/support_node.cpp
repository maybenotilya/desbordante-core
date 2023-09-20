#include "algorithms/md/hymd/model/support_lattice/support_node.h"

#include <cassert>

#include "algorithms/md/hymd/model/get_first_non_zero_index.h"

namespace algos::hymd::model {

bool SupportNode::IsUnsupported(model::SimilarityVector const& lhs_vec,
                                size_t const this_node_index) const {
    if (is_unsupported_) return true;
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const next_node_index = this_node_index + 1 + index;
        assert(next_node_index < lhs_vec.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > lhs_vec[next_node_index]) {
                break;
            }
            if (node->IsUnsupported(lhs_vec, next_node_index)) {
                return true;
            }
        }
    }
    return false;
}

void SupportNode::MarkUnsupported(model::SimilarityVector const& lhs_vec, size_t this_node_index) {
    if (is_unsupported_) return;
    assert(this_node_index < lhs_vec.size());
    size_t const next_node_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index + 1);
    if (next_node_index == lhs_vec.size()) {
        is_unsupported_ = true;
        return;
    }
    assert(next_node_index < lhs_vec.size());
    size_t const child_array_index = next_node_index - (this_node_index + 1);
    model::Similarity const child_similarity = lhs_vec[next_node_index];
    ThresholdMap& threshold_map = children_[child_array_index];
    std::unique_ptr<SupportNode>& node_ptr = threshold_map[child_similarity];
    if (node_ptr == nullptr) {
        node_ptr = std::make_unique<SupportNode>();
    }
    node_ptr->MarkUnsupported(lhs_vec, next_node_index);
}

}