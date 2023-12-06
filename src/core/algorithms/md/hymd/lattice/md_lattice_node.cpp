#include "md_lattice_node.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace algos::hymd::lattice {

void MdLatticeNode::AddUnchecked(DecisionBoundaryVector const& lhs_sims,
                                 model::md::DecisionBoundary rhs_sim, model::Index const rhs_index,
                                 model::Index const this_node_index) {
    assert(children_.empty());
    assert(std::all_of(rhs_.begin(), rhs_.end(),
                       [](model::md::DecisionBoundary const v) { return v == 0.0; }));
    size_t const rhs_size = rhs_.size();
    MdLatticeNode* cur_node_ptr = this;
    for (model::Index cur_node_index = utility::GetFirstNonZeroIndex(lhs_sims, this_node_index);
         cur_node_index != rhs_size;
         cur_node_index = utility::GetFirstNonZeroIndex(lhs_sims, cur_node_index + 1)) {
        cur_node_ptr = &cur_node_ptr->children_[cur_node_index]
                                .emplace(lhs_sims[cur_node_index], rhs_size)
                                .first->second;
    }
    cur_node_ptr->rhs_[rhs_index] = rhs_sim;
}

bool MdLatticeNode::HasGeneralization(DecisionBoundaryVector const& lhs_sims,
                                      model::md::DecisionBoundary rhs_sim,
                                      model::Index const rhs_index,
                                      model::Index const this_node_index) const {
    if (rhs_[rhs_index] >= rhs_sim) return true;
    size_t const rhs_size = rhs_.size();
    for (model::Index cur_index = utility::GetFirstNonZeroIndex(lhs_sims, this_node_index);
         cur_index != rhs_size;
         cur_index = utility::GetFirstNonZeroIndex(lhs_sims, cur_index + 1)) {
        auto it = children_.find(cur_index);
        if (it == children_.end()) continue;
        model::md::DecisionBoundary const child_similarity = lhs_sims[cur_index];
        for (auto const& [threshold, node] : it->second) {
            if (threshold > child_similarity) break;
            if (node.HasGeneralization(lhs_sims, rhs_sim, rhs_index, cur_index + 1)) return true;
        }
    }
    return false;
}

bool MdLatticeNode::AddIfMinimal(DecisionBoundaryVector const& lhs_sims,
                                 model::md::DecisionBoundary rhs_sim, model::Index const rhs_index,
                                 model::Index const this_node_index) {
    model::md::DecisionBoundary& this_rhs_sim = rhs_[rhs_index];
    if (this_rhs_sim >= rhs_sim) return false;
    size_t const rhs_size = rhs_.size();
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_sims, this_node_index);
    if (next_node_index == rhs_size) {
        // Correct. This method is only used when inferring, so we must get a maximum, which is
        // enforced with `if (this_rhs_sim >= rhs_sim) return;` above, no need for an additional
        // check. I believe Metanome's implementation implemented this method incorrectly (they used
        // Math.min instead of Math.max). However, before validating an MD, they set the rhs bound
        // to 1.0, so their error has no effect on the final result. If this is boundary is set
        // correctly (like here), we don't have to set the rhs bound to 1.0 before validating and
        // can start with the value originally in the lattice, as the only way that value could be
        // not equal to 1.0 at the start of the validation is if it was lowered by some record pair.
        this_rhs_sim = rhs_sim;
        // TODO: if rhs_sim is 1, we can remove all specializations, check performance
        //  (if we do, then we can avoid checking for all 1 in GetMaxValidGeneralizationRhs)
        return true;
    }
    {
        auto children_end = children_.end();
        for (model::Index child_node_index =
                     utility::GetFirstNonZeroIndex(lhs_sims, next_node_index + 1);
             child_node_index != rhs_size;
             child_node_index = utility::GetFirstNonZeroIndex(lhs_sims, child_node_index + 1)) {
            auto it = children_.find(child_node_index);
            if (it == children_end) continue;
            model::md::DecisionBoundary const child_lhs_sim = lhs_sims[child_node_index];
            for (auto const& [threshold, node] : it->second) {
                if (threshold > child_lhs_sim) break;
                if (node.HasGeneralization(lhs_sims, rhs_sim, rhs_index, child_node_index + 1))
                    return false;
            }
        }
    }
    auto [it_arr, is_first_arr] = children_.try_emplace(next_node_index);
    ThresholdMap<MdLatticeNode>& threshold_mapping = it_arr->second;
    model::md::DecisionBoundary const next_lhs_sim = lhs_sims[next_node_index];
    if (is_first_arr) [[unlikely]] {
        threshold_mapping.emplace(next_lhs_sim, rhs_size)
                .first->second.AddUnchecked(lhs_sims, rhs_sim, rhs_index, next_node_index + 1);
        return true;
    }
    auto it = threshold_mapping.begin();
    auto end = threshold_mapping.end();
    for (; it != end; ++it) {
        auto& [threshold, node] = *it;
        if (threshold > next_lhs_sim) {
            break;
        }
        if (threshold == next_lhs_sim) {
            return node.AddIfMinimal(lhs_sims, rhs_sim, rhs_index, next_node_index + 1);
        }
        if (node.HasGeneralization(lhs_sims, rhs_sim, rhs_index, next_node_index + 1)) {
            return false;
        }
    }
    threshold_mapping.emplace_hint(it, next_lhs_sim, rhs_size)
            ->second.AddUnchecked(lhs_sims, rhs_sim, rhs_index, next_node_index + 1);
    return true;
}

void MdLatticeNode::FindViolated(std::vector<MdLatticeNodeInfo>& found,
                                 DecisionBoundaryVector& this_node_lhs,
                                 SimilarityVector const& similarity_vector,
                                 [[maybe_unused]] model::Index const this_node_index) {
    {
        assert(rhs_.size() == similarity_vector.size());
        auto it_rhs = rhs_.begin();
        auto it_sim = similarity_vector.begin();
        auto end_rhs = rhs_.end();
        for (; it_rhs != end_rhs; ++it_rhs, ++it_sim) {
            if (*it_sim < *it_rhs) {
                found.emplace_back(this_node_lhs, &rhs_);
                break;
            }
        }
    }
    for (auto& [index, threshold_mapping] : children_) {
        model::md::DecisionBoundary& cur_lhs_sim = this_node_lhs[index];
        model::md::DecisionBoundary const sim_vec_sim = similarity_vector[index];
        assert(index < similarity_vector.size());
        for (auto& [threshold, node] : threshold_mapping) {
            if (threshold > sim_vec_sim) break;
            cur_lhs_sim = threshold;
            node.FindViolated(found, this_node_lhs, similarity_vector, index + 1);
        }
        cur_lhs_sim = 0.0;
    }
}

void MdLatticeNode::RaiseInterestingnessBounds(DecisionBoundaryVector const& lhs,
                                               std::vector<model::md::DecisionBoundary>& cur_rhs,
                                               model::Index const this_node_index,
                                               std::vector<model::Index> const& indices,
                                               std::size_t& ones) const {
    std::size_t const ind_size = indices.size();
    {
        for (std::size_t i = 0; i < ind_size; ++i) {
            model::md::DecisionBoundary const this_node_bound = rhs_[indices[i]];
            model::md::DecisionBoundary& cur = cur_rhs[i];
            if (this_node_bound > cur) {
                cur = this_node_bound;
                if (cur == 1.0) ++ones;
            }
        }
    }
    if (ones == ind_size) return;

    auto child_array_end = children_.end();
    for (model::Index cur_node_index = utility::GetFirstNonZeroIndex(lhs, this_node_index);
         cur_node_index != lhs.size();
         cur_node_index = utility::GetFirstNonZeroIndex(lhs, cur_node_index + 1)) {
        auto it = children_.find(cur_node_index);
        if (it == child_array_end) continue;
        model::md::DecisionBoundary const cur_lhs_sim = lhs[cur_node_index];
        for (auto const& [threshold, node] : it->second) {
            if (threshold > cur_lhs_sim) {
                break;
            }
            node.RaiseInterestingnessBounds(lhs, cur_rhs, cur_node_index + 1, indices, ones);
            if (ones == ind_size) return;
        }
    }
}

void MdLatticeNode::GetLevel(std::vector<MdLatticeNodeInfo>& collected,
                             DecisionBoundaryVector& this_node_lhs,
                             [[maybe_unused]] model::Index this_node_index, size_t level_left,
                             SingleLevelFunc const& single_level_func) {
    if (level_left == 0) {
        if (std::any_of(rhs_.begin(), rhs_.end(),
                        [](model::md::DecisionBoundary val) { return val != 0.0; }))
            collected.emplace_back(this_node_lhs, &rhs_);
        return;
    }
    for (auto& [index, threshold_mapping] : children_) {
        assert(index < this_node_lhs.size());
        for (auto& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            std::size_t single = single_level_func(index, threshold);
            if (single > level_left) break;
            this_node_lhs[index] = threshold;
            node.GetLevel(collected, this_node_lhs, index + 1, level_left - single,
                          single_level_func);
        }
        this_node_lhs[index] = 0.0;
    }
}

}  // namespace algos::hymd::lattice
