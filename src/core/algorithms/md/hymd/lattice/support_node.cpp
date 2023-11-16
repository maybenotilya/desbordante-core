#include "algorithms/md/hymd/lattice/support_node.h"

#include <cassert>
#include <memory>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace algos::hymd::lattice {

void SupportNode::MarkUnchecked(DecisionBoundaryVector const& lhs_vec, model::Index this_node_index) {
    assert(children_.empty());
    size_t const size = lhs_vec.size();
    SupportNode* cur_node_ptr = this;
    for (model::Index cur_node_index = utility::GetFirstNonZeroIndex(lhs_vec, this_node_index);
         cur_node_index != size;
         cur_node_index = utility::GetFirstNonZeroIndex(lhs_vec, cur_node_index + 1)) {
        cur_node_ptr = &cur_node_ptr->children_[cur_node_index]
                                .try_emplace(lhs_vec[cur_node_index])
                                .first->second;
    }
    cur_node_ptr->is_unsupported_ = true;
}

bool SupportNode::IsUnsupported(DecisionBoundaryVector const& lhs_vec,
                                [[maybe_unused]] model::Index const this_node_index) const {
    if (is_unsupported_) return true;
    for (auto const& [index, threshold_mapping] : children_) {
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > lhs_vec[index]) {
                break;
            }
            if (node.IsUnsupported(lhs_vec, index + 1)) {
                return true;
            }
        }
    }
    return false;
}

void SupportNode::MarkUnsupported(DecisionBoundaryVector const& lhs_vec,
                                  model::Index const this_node_index) {
    assert(this_node_index <= lhs_vec.size());
    model::Index const next_node_index =
            utility::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (next_node_index == lhs_vec.size()) {
        is_unsupported_ = true;
        return;
    }
    assert(next_node_index < lhs_vec.size());
    model::md::DecisionBoundary const child_similarity = lhs_vec[next_node_index];
    auto [it_arr, is_first] = children_.try_emplace(next_node_index);
    ThresholdMap<SupportNode>& threshold_map = it_arr->second;
    if (is_first) {
        threshold_map.try_emplace(child_similarity).first->second.MarkUnchecked(lhs_vec, next_node_index + 1);
        return;
    }
    auto [it_map, is_first_map] = threshold_map.try_emplace(child_similarity);
    SupportNode& next_node = it_map->second;
    if (is_first_map) {
        next_node.MarkUnchecked(lhs_vec, next_node_index + 1);
        return;
    }
    next_node.MarkUnsupported(lhs_vec, next_node_index + 1);
}

}  // namespace algos::hymd::lattice
