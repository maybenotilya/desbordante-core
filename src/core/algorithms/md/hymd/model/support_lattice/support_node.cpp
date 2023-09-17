#include "algorithms/md/hymd/model/support_lattice/support_node.h"

#include <cassert>

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
    assert(this_node_index <= lhs_vec.size());
    size_t const child_array_size_limit = lhs_vec.size() - this_node_index;
    for (size_t child_array_index = 0; child_array_index < child_array_size_limit;
         ++child_array_index) {
        size_t const sim_index = this_node_index + child_array_index;
        size_t const new_node_index = sim_index + 1;
        assert(sim_index < lhs_vec.size());
        model::Similarity similarity = lhs_vec[sim_index];
        if (similarity != 0.0) {
            ThresholdMap& threshold_map = children_[child_array_index];
            threshold_map[similarity]->MarkUnsupported(lhs_vec, new_node_index);
            break;
        }
    }
    is_unsupported_ = true;
}

}