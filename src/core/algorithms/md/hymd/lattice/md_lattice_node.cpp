#include "md_lattice_node.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace algos::hymd::lattice {

void MdLatticeNode::AddUnchecked(DecisionBoundaryVector const& lhs_bounds,
                                 model::md::DecisionBoundary rhs_bound,
                                 model::Index const rhs_index, model::Index const this_node_index) {
    assert(children_.empty());
    assert(std::all_of(rhs_bounds_.begin(), rhs_bounds_.end(),
                       [](model::md::DecisionBoundary const bound) { return bound == 0.0; }));
    std::size_t const col_match_number = rhs_bounds_.size();
    MdLatticeNode* cur_node_ptr = this;
    for (model::Index cur_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         cur_node_index != col_match_number;
         cur_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, cur_node_index + 1)) {
        cur_node_ptr = &cur_node_ptr->children_.try_emplace(cur_node_index)
                                .first->second.emplace(lhs_bounds[cur_node_index], col_match_number)
                                .first->second;
    }
    cur_node_ptr->rhs_bounds_[rhs_index] = rhs_bound;
}

bool MdLatticeNode::HasGeneralization(DecisionBoundaryVector const& lhs_bounds,
                                      model::md::DecisionBoundary rhs_bound,
                                      model::Index const rhs_index,
                                      model::Index const this_node_index) const {
    if (rhs_bounds_[rhs_index] >= rhs_bound) return true;
    std::size_t const col_match_number = rhs_bounds_.size();
    for (model::Index cur_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         cur_index != col_match_number;
         cur_index = utility::GetFirstNonZeroIndex(lhs_bounds, cur_index + 1)) {
        auto it = children_.find(cur_index);
        if (it == children_.end()) continue;
        model::md::DecisionBoundary const generalization_bound_limit = lhs_bounds[cur_index];
        for (auto const& [generalization_bound, node] : it->second) {
            if (generalization_bound > generalization_bound_limit) break;
            if (node.HasGeneralization(lhs_bounds, rhs_bound, rhs_index, cur_index + 1))
                return true;
        }
    }
    return false;
}

bool MdLatticeNode::AddIfMinimal(DecisionBoundaryVector const& lhs_bounds,
                                 model::md::DecisionBoundary rhs_bound,
                                 model::Index const rhs_index, model::Index const this_node_index) {
    model::md::DecisionBoundary& this_node_rhs_bound = rhs_bounds_[rhs_index];
    if (this_node_rhs_bound >= rhs_bound) return false;
    std::size_t const col_match_number = rhs_bounds_.size();
    model::Index const next_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
    if (next_node_index == col_match_number) {
        // Correct. This method is only used when inferring, so we must get a maximum, which is
        // enforced with `if (this_rhs_sim >= rhs_sim) return;` above, no need for an additional
        // check. I believe Metanome implemented this method incorrectly (they used Math.min instead
        // of Math.max). However, before validating an MD, they set the rhs bound to 1.0, so that
        // error has no effect on the final result. If this boundary is set correctly (like here),
        // we don't have to set the RHS bound to 1.0 before validating and can start with the value
        // that was originally in the lattice, as the only way that value could be not equal to 1.0
        // at the start of the validation is if it was lowered by some record pair.
        this_node_rhs_bound = rhs_bound;
        return true;
    }
    {
        auto children_end = children_.end();
        for (model::Index child_node_index =
                     utility::GetFirstNonZeroIndex(lhs_bounds, next_node_index + 1);
             child_node_index != col_match_number;
             child_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, child_node_index + 1)) {
            auto it = children_.find(child_node_index);
            if (it == children_end) continue;
            model::md::DecisionBoundary const generalization_bound_limit =
                    lhs_bounds[child_node_index];
            for (auto const& [generalization_bound, node] : it->second) {
                if (generalization_bound > generalization_bound_limit) break;
                if (node.HasGeneralization(lhs_bounds, rhs_bound, rhs_index, child_node_index + 1))
                    return false;
            }
        }
    }
    auto [it_arr, is_first_arr] = children_.try_emplace(next_node_index);
    BoundaryMap<MdLatticeNode>& boundary_mapping = it_arr->second;
    model::md::DecisionBoundary const next_lhs_generalization_bound_limit =
            lhs_bounds[next_node_index];
    if (is_first_arr) [[unlikely]] {
        boundary_mapping.emplace(next_lhs_generalization_bound_limit, col_match_number)
                .first->second.AddUnchecked(lhs_bounds, rhs_bound, rhs_index, next_node_index + 1);
        return true;
    }
    auto it = boundary_mapping.begin();
    auto end = boundary_mapping.end();
    for (; it != end; ++it) {
        auto& [generalization_bound, node] = *it;
        if (generalization_bound > next_lhs_generalization_bound_limit) {
            break;
        }
        if (generalization_bound == next_lhs_generalization_bound_limit) {
            return node.AddIfMinimal(lhs_bounds, rhs_bound, rhs_index, next_node_index + 1);
        }
        if (node.HasGeneralization(lhs_bounds, rhs_bound, rhs_index, next_node_index + 1)) {
            return false;
        }
    }
    boundary_mapping.emplace_hint(it, next_lhs_generalization_bound_limit, col_match_number)
            ->second.AddUnchecked(lhs_bounds, rhs_bound, rhs_index, next_node_index + 1);
    return true;
}

void MdLatticeNode::FindViolated(std::vector<MdLatticeNodeInfo>& found,
                                 DecisionBoundaryVector& this_node_lhs_bounds,
                                 SimilarityVector const& similarity_vector,
                                 [[maybe_unused]] model::Index const this_node_index) {
    {
        assert(rhs_bounds_.size() == similarity_vector.size());
        auto it_rhs = rhs_bounds_.begin();
        auto it_sim = similarity_vector.begin();
        auto end_rhs = rhs_bounds_.end();
        for (; it_rhs != end_rhs; ++it_rhs, ++it_sim) {
            if (*it_sim < *it_rhs) {
                found.emplace_back(this_node_lhs_bounds, &rhs_bounds_);
                break;
            }
        }
    }
    for (auto& [index, boundary_mapping] : children_) {
        model::md::DecisionBoundary& cur_lhs_bound = this_node_lhs_bounds[index];
        model::md::DecisionBoundary const sim_vec_sim = similarity_vector[index];
        assert(index < similarity_vector.size());
        for (auto& [generalization_bound, node] : boundary_mapping) {
            if (generalization_bound > sim_vec_sim) break;
            cur_lhs_bound = generalization_bound;
            node.FindViolated(found, this_node_lhs_bounds, similarity_vector, index + 1);
        }
        cur_lhs_bound = 0.0;
    }
}

void MdLatticeNode::RaiseInterestingnessBounds(
        DecisionBoundaryVector const& lhs_bounds,
        std::vector<model::md::DecisionBoundary>& cur_interestingness_bounds,
        model::Index const this_node_index, std::vector<model::Index> const& indices) const {
    {
        std::size_t const indices_size = indices.size();
        for (model::Index i = 0; i < indices_size; ++i) {
            model::md::DecisionBoundary const this_node_bound = rhs_bounds_[indices[i]];
            model::md::DecisionBoundary& cur = cur_interestingness_bounds[i];
            if (this_node_bound > cur) {
                cur = this_node_bound;
                // The original paper mentions checking for the case where all decision bounds are
                // 1.0, but if such a situation occurs for any one RHS, and the generalization with
                // that RHS happens to be valid on the data, it would make inference from record
                // pairs give an incorrect result, meaning the algorithm is incorrect.
                // However, it is possible to stop decreasing when the bound's index in the list of
                // natural decision boundaries is exactly one less than the RHS bound's index.
                assert(this_node_bound != 1.0);
            }
        }
    }

    auto child_array_end = children_.end();
    std::size_t const col_match_number = lhs_bounds.size();
    for (model::Index cur_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         cur_node_index != col_match_number;
         cur_node_index = utility::GetFirstNonZeroIndex(lhs_bounds, cur_node_index + 1)) {
        auto it = children_.find(cur_node_index);
        if (it == child_array_end) continue;
        model::md::DecisionBoundary const generalization_bound_limit = lhs_bounds[cur_node_index];
        for (auto const& [generalization_bound, node] : it->second) {
            if (generalization_bound > generalization_bound_limit) break;
            node.RaiseInterestingnessBounds(lhs_bounds, cur_interestingness_bounds,
                                            cur_node_index + 1, indices);
        }
    }
}

void MdLatticeNode::GetLevel(std::vector<MdLatticeNodeInfo>& collected,
                             DecisionBoundaryVector& this_node_lhs_bounds,
                             [[maybe_unused]] model::Index this_node_index, std::size_t level_left,
                             SingleLevelFunc const& single_level_func) {
    if (level_left == 0) {
        if (std::any_of(rhs_bounds_.begin(), rhs_bounds_.end(),
                        [](model::md::DecisionBoundary bound) { return bound != 0.0; }))
            collected.emplace_back(this_node_lhs_bounds, &rhs_bounds_);
        return;
    }
    for (auto& [index, boundary_mapping] : children_) {
        assert(index < this_node_lhs_bounds.size());
        model::md::DecisionBoundary& current_lhs_bound = this_node_lhs_bounds[index];
        for (auto& [boundary, node] : boundary_mapping) {
            assert(boundary > 0.0);
            std::size_t single = single_level_func(index, boundary);
            if (single > level_left) break;
            current_lhs_bound = boundary;
            node.GetLevel(collected, this_node_lhs_bounds, index + 1, level_left - single,
                          single_level_func);
        }
        current_lhs_bound = 0.0;
    }
}

}  // namespace algos::hymd::lattice
