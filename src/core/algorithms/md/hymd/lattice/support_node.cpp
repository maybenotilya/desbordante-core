#include "algorithms/md/hymd/lattice/support_node.h"

#include <cassert>
#include <memory>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace algos::hymd::lattice {

bool SupportNode::IsUnsupported(DecisionBoundaryVector const& lhs_vec,
                                model::Index const this_node_index) const {
    if (is_unsupported_) return true;
    for (auto const& [index, threshold_mapping] : children_) {
        model::Index const cur_node_index = this_node_index + index;
        assert(cur_node_index < lhs_vec.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > lhs_vec[cur_node_index]) {
                break;
            }
            if (node->IsUnsupported(lhs_vec, cur_node_index + 1)) {
                return true;
            }
        }
    }
    return false;
}

void SupportNode::MarkUnsupported(DecisionBoundaryVector const& lhs_vec,
                                  model::Index const this_node_index) {
    assert(this_node_index <= lhs_vec.size());
    model::Index const first_non_zero_index =
            utility::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (first_non_zero_index == lhs_vec.size()) {
        is_unsupported_ = true;
        return;
    }
    assert(first_non_zero_index < lhs_vec.size());
    model::Index const child_array_index = first_non_zero_index - this_node_index;
    model::md::DecisionBoundary const child_similarity = lhs_vec[first_non_zero_index];
    ThresholdMap<SupportNode>& threshold_map = children_[child_array_index];
    std::unique_ptr<SupportNode>& node_ptr = threshold_map[child_similarity];
    if (node_ptr == nullptr) {
        node_ptr = std::make_unique<SupportNode>();
    }
    node_ptr->MarkUnsupported(lhs_vec, first_non_zero_index + 1);
}

}  // namespace algos::hymd::lattice
