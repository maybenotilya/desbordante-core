#include "md_lattice_node.h"

#include <cassert>

namespace algos::hymd::model {

void MdLatticeNode::Add(LatticeMd const& md, size_t const this_node_index) {
    model::SimilarityVector const& lhs_vec = md.lhs_sims;
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
            std::unique_ptr<MdLatticeNode>& node_ptr = threshold_map[similarity];
            if (node_ptr == nullptr) {
                node_ptr = std::make_unique<MdLatticeNode>(rhs_.size());
            }
            node_ptr->Add(md, new_node_index);
            return;
        }
    }
    double& cur_sim = rhs_[md.rhs_index];
    double const added_md_sim = md.rhs_sim;
    if (added_md_sim < cur_sim) {
        cur_sim = added_md_sim;
    }
}

bool MdLatticeNode::AddIfMin(LatticeMd const& md) {
    if (HasGeneralization(md, 0)) return false;
    Add(md, 0);
    return true;
}

bool MdLatticeNode::HasGeneralization(LatticeMd const& md, size_t this_node_index) {
    if (rhs_[md.rhs_index] >= md.rhs_sim) return true;
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

void MdLatticeNode::RemoveMd(LatticeMd const& md, size_t const this_node_index) {
    model::SimilarityVector const& lhs_vec = md.lhs_sims;
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
            std::unique_ptr<MdLatticeNode>& node_ptr = threshold_map[similarity];
            assert(node_ptr != nullptr);
            node_ptr->RemoveMd(md, new_node_index);
            return;
        }
    }
    double& cur_sim = rhs_[md.rhs_index];
    double const removed_md_sim = md.rhs_sim;
    assert(cur_sim != 0.0 && cur_sim == removed_md_sim);
    cur_sim = 0.0;
}

void MdLatticeNode::RemoveNode(SimilarityVector const& lhs_vec, size_t this_node_index) {
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
            std::unique_ptr<MdLatticeNode>& node_ptr = threshold_map[similarity];
            assert(node_ptr != nullptr);
            node_ptr->RemoveNode(lhs_vec, new_node_index);
            return;
        }
    }
    rhs_.assign(rhs_.size(), 0.0);
}

void MdLatticeNode::FindViolated(std::vector<LatticeMd>& found, SimilarityVector& this_node_lhs,
                                 SimilarityVector const& similarity_vector,
                                 size_t this_node_index) {
    // check rhs
    for (size_t i = 0; i < rhs_.size(); ++i) {
        double const assumed_rhs = rhs_[i];
        if (similarity_vector[i] < assumed_rhs) {
            found.emplace_back(this_node_lhs, assumed_rhs, i);
        }
    }
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const next_node_index = this_node_index + 1 + index;
        assert(next_node_index < similarity_vector.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > similarity_vector[next_node_index]) {
                break;
            }
            this_node_lhs[next_node_index] = threshold;
            node->FindViolated(found, this_node_lhs, similarity_vector, next_node_index);
            this_node_lhs[next_node_index] = 0.0;
        }
    }
}

}
