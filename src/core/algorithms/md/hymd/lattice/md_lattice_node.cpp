#include "md_lattice_node.h"

#include <algorithm>
#include <cassert>
#include <optional>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace {
using namespace algos::hymd::lattice;
using namespace algos::hymd;
using BoundMap = BoundaryMap<MdLatticeNode>;
using OptionalChild = std::optional<BoundMap>;
using model::md::DecisionBoundary, model::Index;
using utility::GetFirstNonZeroIndex;

bool NotEmpty(DecisionBoundaryVector const& rhs_bounds) {
    auto not_lowest = [](DecisionBoundary bound) { return bound != kLowestBound; };
    return std::any_of(rhs_bounds.begin(), rhs_bounds.end(), not_lowest);
}
}  // namespace

namespace algos::hymd::lattice {

void MdLatticeNode::AddUnchecked(DecisionBoundaryVector const& lhs_bounds,
                                 DecisionBoundary rhs_bound, Index const rhs_index,
                                 Index this_node_index) {
    assert(IsEmpty(children_));
    assert(!NotEmpty(rhs_bounds_));
    std::size_t const col_match_number = rhs_bounds_.size();
    MdLatticeNode* cur_node_ptr = this;
    for (Index next_node_index = GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         next_node_index != col_match_number; this_node_index = next_node_index + 1,
               next_node_index = GetFirstNonZeroIndex(lhs_bounds, this_node_index)) {
        std::size_t const child_array_index = next_node_index - this_node_index;
        std::size_t const next_child_array_size = col_match_number - next_node_index;
        cur_node_ptr = &cur_node_ptr->children_[child_array_index]
                                .emplace()
                                .try_emplace(lhs_bounds[next_node_index], col_match_number,
                                             next_child_array_size)
                                .first->second;
    }
    cur_node_ptr->rhs_bounds_[rhs_index] = rhs_bound;
}

bool MdLatticeNode::HasGeneralization(DecisionBoundaryVector const& lhs_bounds,
                                      DecisionBoundary rhs_bound, Index const rhs_index,
                                      Index const this_node_index) const {
    if (rhs_bounds_[rhs_index] >= rhs_bound) return true;
    std::size_t const col_match_number = rhs_bounds_.size();
    for (Index next_node_index = GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         next_node_index != col_match_number;
         next_node_index = GetFirstNonZeroIndex(lhs_bounds, next_node_index + 1)) {
        Index const child_array_index = next_node_index - this_node_index;
        OptionalChild const& optional_child = children_[child_array_index];
        if (!optional_child.has_value()) continue;
        DecisionBoundary const generalization_bound_limit = lhs_bounds[next_node_index];
        for (auto const& [generalization_bound, node] : *optional_child) {
            if (generalization_bound > generalization_bound_limit) break;
            if (node.HasGeneralization(lhs_bounds, rhs_bound, rhs_index, next_node_index + 1))
                return true;
        }
    }
    return false;
}

bool MdLatticeNode::AddIfMinimal(DecisionBoundaryVector const& lhs_bounds,
                                 DecisionBoundary const rhs_bound, Index const rhs_index,
                                 Index this_node_index) {
    std::size_t const col_match_number = rhs_bounds_.size();

    MdLatticeNode* cur_node_ptr = this;
    DecisionBoundary& this_node_rhs_bound = cur_node_ptr->rhs_bounds_[rhs_index];
    if (this_node_rhs_bound >= rhs_bound) return false;
    DecisionBoundary* cur_node_rhs_ptr = &this_node_rhs_bound;

    for (Index next_node_index = GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         next_node_index != col_match_number; this_node_index = next_node_index + 1,
               next_node_index = GetFirstNonZeroIndex(lhs_bounds, this_node_index)) {
        LatticeChildArray<MdLatticeNode>& children = cur_node_ptr->children_;
        for (Index fol_next_node_index = GetFirstNonZeroIndex(lhs_bounds, next_node_index + 1);
             fol_next_node_index != col_match_number;
             fol_next_node_index = GetFirstNonZeroIndex(lhs_bounds, fol_next_node_index + 1)) {
            std::size_t const child_array_index = fol_next_node_index - this_node_index;
            OptionalChild& optional_child = children[child_array_index];
            if (!optional_child.has_value()) continue;
            DecisionBoundary const generalization_bound_limit = lhs_bounds[fol_next_node_index];
            for (auto const& [generalization_bound, node] : *optional_child) {
                if (generalization_bound > generalization_bound_limit) break;
                if (node.HasGeneralization(lhs_bounds, rhs_bound, rhs_index,
                                           fol_next_node_index + 1))
                    return false;
            }
        }

        std::size_t const child_array_index = next_node_index - this_node_index;
        auto [boundary_mapping, is_first_arr] = TryEmplaceChild(children, child_array_index);
        DecisionBoundary const next_lhs_bound = lhs_bounds[next_node_index];
        std::size_t const next_child_array_size = col_match_number - next_node_index;
        if (is_first_arr) [[unlikely]] {
            boundary_mapping.try_emplace(next_lhs_bound, col_match_number, next_child_array_size)
                    .first->second.AddUnchecked(lhs_bounds, rhs_bound, rhs_index,
                                                next_node_index + 1);
            return true;
        }

        auto it = boundary_mapping.begin();
        auto end = boundary_mapping.end();
        for (; it != end; ++it) {
            auto& [generalization_bound, node] = *it;
            if (generalization_bound > next_lhs_bound) {
                break;
            }
            if (generalization_bound == next_lhs_bound) {
                goto advance;
            }
            if (node.HasGeneralization(lhs_bounds, rhs_bound, rhs_index, next_node_index + 1)) {
                return false;
            }
        }
        // first in bound map
        boundary_mapping
                .emplace_hint(it, std::piecewise_construct, std::forward_as_tuple(next_lhs_bound),
                              std::forward_as_tuple(col_match_number, next_child_array_size))
                ->second.AddUnchecked(lhs_bounds, rhs_bound, rhs_index, next_node_index + 1);
        return true;

    advance:
        cur_node_ptr = &it->second;
        DecisionBoundary& cur_node_rhs_bound = cur_node_ptr->rhs_bounds_[rhs_index];
        if (cur_node_rhs_bound >= rhs_bound) return false;
        cur_node_rhs_ptr = &cur_node_rhs_bound;
    }
    // Correct. This method is only used when inferring, so we must get a maximum, which is
    // enforced with either of `if (cur_node_rhs_bound >= rhs_bound) return false;` or
    // `if (this_node_rhs_bound >= rhs_bound) return false;` above, no need for an additional check.
    // I believe Metanome implemented this method incorrectly (they used Math.min instead of
    // Math.max). However, before validating an MD, they set the rhs bound to 1.0, so that error has
    // no effect on the final result. If this boundary is set correctly (like here), we don't have
    // to set the RHS bound to 1.0 before validating and can start with the value that was
    // originally in the lattice, potentially yielding more recommendations, since the only way that
    // value could be not equal to 1.0 at the start of validation is if it was lowered by some
    // record pair.
    *cur_node_rhs_ptr = rhs_bound;
    return true;
}

void MdLatticeNode::FindViolated(std::vector<MdLatticeNodeInfo>& found,
                                 DecisionBoundaryVector& this_node_lhs_bounds,
                                 SimilarityVector const& similarity_vector,
                                 Index const this_node_index) {
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
    std::size_t const child_array_size = children_.size();
    for (Index child_array_index = FindFirstNonEmptyIndex(children_, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(children_, child_array_index + 1)) {
        Index const next_node_index = this_node_index + child_array_index;
        DecisionBoundary& cur_lhs_bound = this_node_lhs_bounds[next_node_index];
        DecisionBoundary const sim_vec_sim = similarity_vector[next_node_index];
        for (auto& [generalization_bound, node] : *children_[child_array_index]) {
            if (generalization_bound > sim_vec_sim) break;
            cur_lhs_bound = generalization_bound;
            node.FindViolated(found, this_node_lhs_bounds, similarity_vector, next_node_index + 1);
        }
        cur_lhs_bound = kLowestBound;
    }
}

void MdLatticeNode::RaiseInterestingnessBounds(
        DecisionBoundaryVector const& lhs_bounds,
        std::vector<DecisionBoundary>& cur_interestingness_bounds, Index const this_node_index,
        std::vector<Index> const& indices) const {
    {
        std::size_t const indices_size = indices.size();
        for (Index i = 0; i < indices_size; ++i) {
            DecisionBoundary const this_node_bound = rhs_bounds_[indices[i]];
            DecisionBoundary& cur = cur_interestingness_bounds[i];
            if (this_node_bound > cur) {
                cur = this_node_bound;
                // The original paper mentions checking for the case where all decision bounds are
                // 1.0, but if such a situation occurs for any one RHS, and the generalization with
                // that RHS happens to be valid on the data, it would make inference from record
                // pairs give an incorrect result, meaning the algorithm is incorrect.
                // However, it is possible to stop decreasing when the bound's index in the list of
                // natural decision boundaries is exactly one less than the RHS bound's index.
                // TODO: abort traversal as above.
                assert(this_node_bound != 1.0);
            }
        }
    }

    std::size_t const col_match_number = lhs_bounds.size();
    for (Index next_node_index = GetFirstNonZeroIndex(lhs_bounds, this_node_index);
         next_node_index != col_match_number;
         next_node_index = GetFirstNonZeroIndex(lhs_bounds, next_node_index + 1)) {
        Index const child_array_index = next_node_index - this_node_index;
        OptionalChild const& optional_child = children_[child_array_index];
        if (!optional_child.has_value()) continue;
        DecisionBoundary const generalization_bound_limit = lhs_bounds[next_node_index];
        for (auto const& [generalization_bound, node] : *optional_child) {
            if (generalization_bound > generalization_bound_limit) break;
            node.RaiseInterestingnessBounds(lhs_bounds, cur_interestingness_bounds,
                                            next_node_index + 1, indices);
        }
    }
}

void MdLatticeNode::GetLevel(std::vector<MdLatticeNodeInfo>& collected,
                             DecisionBoundaryVector& this_node_lhs_bounds, Index this_node_index,
                             std::size_t level_left, SingleLevelFunc const& single_level_func) {
    if (level_left == 0) {
        if (NotEmpty(rhs_bounds_)) collected.emplace_back(this_node_lhs_bounds, &rhs_bounds_);
        return;
    }
    std::size_t const child_array_size = children_.size();
    for (Index child_array_index = FindFirstNonEmptyIndex(children_, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(children_, child_array_index + 1)) {
        Index const next_node_index = this_node_index + child_array_index;
        DecisionBoundary& next_lhs_bound = this_node_lhs_bounds[next_node_index];
        for (auto& [boundary, node] : *children_[child_array_index]) {
            assert(boundary > kLowestBound);
            std::size_t single = single_level_func(next_node_index, boundary);
            if (single > level_left) break;
            next_lhs_bound = boundary;
            node.GetLevel(collected, this_node_lhs_bounds, next_node_index + 1, level_left - single,
                          single_level_func);
        }
        next_lhs_bound = kLowestBound;
    }
}

void MdLatticeNode::GetAll(std::vector<MdLatticeNodeInfo>& collected,
                           DecisionBoundaryVector& this_node_lhs_bounds, Index this_node_index) {
    if (NotEmpty(rhs_bounds_)) collected.emplace_back(this_node_lhs_bounds, &rhs_bounds_);
    std::size_t const child_array_size = children_.size();
    for (Index child_array_index = FindFirstNonEmptyIndex(children_, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(children_, child_array_index + 1)) {
        Index const next_node_index = this_node_index + child_array_index;
        DecisionBoundary& next_lhs_bound = this_node_lhs_bounds[next_node_index];
        for (auto& [boundary, node] : *children_[child_array_index]) {
            next_lhs_bound = boundary;
            node.GetAll(collected, this_node_lhs_bounds, next_node_index + 1);
        }
        next_lhs_bound = kLowestBound;
    }
}

}  // namespace algos::hymd::lattice
