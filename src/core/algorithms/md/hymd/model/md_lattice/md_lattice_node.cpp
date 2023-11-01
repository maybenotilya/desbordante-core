#include "md_lattice_node.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/hymd/model/get_first_non_zero_index.h"

namespace algos::hymd::model {

void MdLatticeNode::Add(SimilarityVector const& lhs_sims, Similarity rhs_sim, size_t rhs_index,
                        size_t const this_node_index) {
    assert(this_node_index <= lhs_sims.size());
    size_t const first_non_zero_index = util::GetFirstNonZeroIndex(lhs_sims, this_node_index);
    if (first_non_zero_index == lhs_sims.size()) {
        double& cur_sim = rhs_[rhs_index];
        assert(rhs_sim != 0.0);
        if (cur_sim == 0.0 || rhs_sim < cur_sim) {
            cur_sim = rhs_sim;
        }
        return;
    }
    assert(first_non_zero_index < lhs_vec.size());
    size_t const child_array_index = first_non_zero_index - this_node_index;
    model::Similarity const child_similarity = lhs_sims[first_non_zero_index];
    ThresholdMap& threshold_map = children_[child_array_index];
    std::unique_ptr<MdLatticeNode>& node_ptr = threshold_map[child_similarity];
    if (node_ptr == nullptr) {
        node_ptr = std::make_unique<MdLatticeNode>(rhs_.size());
    }
    node_ptr->Add(lhs_sims, rhs_sim, rhs_index, first_non_zero_index + 1);
}

bool MdLatticeNode::HasGeneralization(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                      size_t rhs_index, size_t this_node_index) const {
    if (rhs_[rhs_index] >= rhs_sim) return true;
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const cur_node_index = this_node_index + index;
        assert(cur_node_index < lhs_vec.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > lhs_sims[cur_node_index]) {
                break;
            }
            if (node->HasGeneralization(lhs_sims, rhs_sim, rhs_index, cur_node_index + 1)) {
                return true;
            }
        }
    }
    return false;
}

void MdLatticeNode::RemoveNode(SimilarityVector const& lhs_vec, size_t this_node_index) {
    assert(this_node_index <= lhs_vec.size());
    size_t const next_node_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (next_node_index == lhs_vec.size()) {
        rhs_.assign(rhs_.size(), 0.0);
        return;
    }
    assert(next_node_index < lhs_vec.size());
    size_t const child_array_index = next_node_index - this_node_index;
    model::Similarity const child_similarity = lhs_vec[next_node_index];
    ThresholdMap& threshold_map = children_[child_array_index];
    std::unique_ptr<MdLatticeNode>& node_ptr = threshold_map[child_similarity];
    assert(node_ptr != nullptr);
    node_ptr->RemoveNode(lhs_vec, next_node_index + 1);
}

void MdLatticeNode::FindViolated(std::vector<MdLatticeNodeInfo>& found,
                                 SimilarityVector& this_node_lhs,
                                 SimilarityVector const& similarity_vector,
                                 size_t this_node_index) {
    size_t const col_matches = similarity_vector.size();
    {
        assert(rhs_.size() == similarity_vector.size());
        for (size_t i = 0; i < col_matches; ++i) {
            if (similarity_vector[i] < rhs_[i]) {
                found.emplace_back(this_node_lhs, &rhs_);
            }
        }
        /*
        auto it_rhs = rhs_.begin();
        auto it_sim = similarity_vector.begin();
        auto end_rhs = rhs_.end();
        for (; it_rhs != end_rhs; ++it_rhs, ++it_sim) {
            if (*it_sim < *it_rhs) {
                found.emplace_back(this_node_lhs, &rhs_);
                break;
            }
        }
        */
    }
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const cur_node_index = this_node_index + index;
        Similarity& cur_lhs_sim = this_node_lhs[cur_node_index];
        Similarity const sim_vec_sim = similarity_vector[cur_node_index];
        assert(cur_node_index < similarity_vector.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > sim_vec_sim) {
                break;
            }
            cur_lhs_sim = threshold;
            node->FindViolated(found, this_node_lhs, similarity_vector, cur_node_index + 1);
        }
        cur_lhs_sim = 0.0;
    }
}

void MdLatticeNode::GetMaxValidGeneralizationRhs(SimilarityVector const& lhs,
                                                 SimilarityVector& cur_rhs,
                                                 size_t this_node_index) const {
    for (size_t i = 0; i < rhs_.size(); ++i) {
        double const rhs_threshold = rhs_[i];
        double& cur_rhs_threshold = cur_rhs[i];
        if (rhs_threshold > cur_rhs_threshold) {
            cur_rhs_threshold = rhs_threshold;
        }
    }

    auto const& child_array_end = children_.end();
    for (size_t cur_node_index = this_node_index;
         (cur_node_index = util::GetFirstNonZeroIndex(lhs, cur_node_index)) != lhs.size();
         ++cur_node_index) {
        size_t const child_array_index = cur_node_index - this_node_index;
        auto it = children_.find(child_array_index);
        if (it == child_array_end) continue;
        for (auto const& [threshold, node] : it->second) {
            if (threshold > lhs[cur_node_index]) {
                break;
            }
            node->GetMaxValidGeneralizationRhs(lhs, cur_rhs, cur_node_index + 1);
        }
    }
    /* alternative: go through all indices, check for zeroes in LHS
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const cur_node_index = this_node_index + index;
        assert(cur_node_index < lhs.size());
        if (lhs[cur_node_index] == 0.0) continue;
        for (auto const& [threshold, node] : threshold_mapping) {
            if (threshold > lhs[cur_node_index]) {
                break;
            }
            node->GetMaxValidGeneralizationRhs(lhs, cur_rhs, cur_node_index + 1);
        }
    }
     */
    /* old way, node has to be removed first (zeroed out RHS).
    // By this point, the RHS of the node for lhs should be zeroed out, so no check is needed.
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const cur_node_index = this_node_index + index;
        assert(cur_node_index < lhs.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > lhs[cur_node_index]) {
                break;
            }
            node->GetMaxValidGeneralizationRhs(lhs, cur_rhs, cur_node_index + 1);
        }
    }
     */
}

void MdLatticeNode::GetLevel(std::vector<LatticeNodeSims>& collected,
                             SimilarityVector& this_node_lhs, size_t this_node_index,
                             size_t sims_left) const {
    if (sims_left == 0) {
        if (std::any_of(rhs_.begin(), rhs_.end(), [](Similarity val) { return val != 0.0; }))
            collected.emplace_back(this_node_lhs, rhs_);
        return;
    }
    for (auto const& [index, threshold_mapping] : children_) {
        size_t const cur_node_index = this_node_index + index;
        assert(cur_node_index < this_node_lhs.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            this_node_lhs[cur_node_index] = threshold;
            node->GetLevel(collected, this_node_lhs, cur_node_index + 1, sims_left - 1);
        }
        this_node_lhs[cur_node_index] = 0.0;
    }
}

}
