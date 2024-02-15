#include "algorithms/md/hymd/lattice/md_lattice.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/utility/get_first_non_zero_index.h"

namespace {
using namespace algos::hymd::lattice;
using namespace algos::hymd;
using model::md::DecisionBoundary, model::Index;
using utility::GetFirstNonZeroIndex;

bool NotEmpty(DecisionBoundaryVector const& rhs_bounds) {
    auto not_lowest = [](DecisionBoundary bound) { return bound != kLowestBound; };
    return std::any_of(rhs_bounds.begin(), rhs_bounds.end(), not_lowest);
}
}  // namespace

namespace algos::hymd::lattice {

// TODO: remove recursion
MdLattice::MdLattice(std::size_t column_matches_size, SingleLevelFunc single_level_func)
    : root_(DecisionBoundaryVector(column_matches_size, 1.0)),
      column_matches_size_(column_matches_size),
      get_single_level_(std::move(single_level_func)) {}

void MdLattice::AddUnchecked(Node& cur_node, DecisionBoundaryVector const& lhs_bounds,
                             DecisionBoundary rhs_bound, Index rhs_index, Index cur_node_index) {
    assert(IsEmpty(cur_node.children));
    assert(!NotEmpty(cur_node.rhs_bounds));
    Node* cur_node_ptr = &cur_node;
    for (Index next_node_index = GetFirstNonZeroIndex(lhs_bounds, cur_node_index);
         next_node_index != column_matches_size_;
         next_node_index = GetFirstNonZeroIndex(lhs_bounds, next_node_index + 1)) {
        std::size_t const child_array_index = next_node_index - cur_node_index;
        std::size_t const next_child_array_size = column_matches_size_ - next_node_index;
        cur_node_ptr = &cur_node_ptr->children[child_array_index]
                                .emplace()
                                .try_emplace(lhs_bounds[next_node_index], column_matches_size_,
                                             next_child_array_size)
                                .first->second;
        cur_node_index = next_node_index + 1;
    }
    cur_node_ptr->rhs_bounds[rhs_index] = rhs_bound;
}

bool MdLattice::HasLhsGeneralization(Node const& node, DecisionBoundaryVector const& lhs_bounds,
                                     DecisionBoundary rhs_bound, Index rhs_index, Index node_index,
                                     Index start_index) const {
    for (Index next_node_index = GetFirstNonZeroIndex(lhs_bounds, start_index);
         next_node_index != column_matches_size_;
         next_node_index = GetFirstNonZeroIndex(lhs_bounds, next_node_index + 1)) {
        Index const child_array_index = next_node_index - node_index;
        OptionalChild const& optional_child = node.children[child_array_index];
        if (!optional_child.has_value()) continue;
        DecisionBoundary const generalization_bound_limit = lhs_bounds[next_node_index];
        for (auto const& [generalization_bound, node] : *optional_child) {
            if (generalization_bound > generalization_bound_limit) break;
            if (HasGeneralization(node, lhs_bounds, rhs_bound, rhs_index, next_node_index + 1))
                return true;
        }
    }
    return false;
}

void MdLattice::AddIfMinimal(DecisionBoundaryVector const& lhs_bounds,
                             DecisionBoundary const rhs_bound, Index rhs_index) {
    // TODO: use info about where the LHS was specialized from to reduce generalization checks.
    // When an MD is inferred from, it is not a specialization of any other MD in the lattice, so
    // its LHS is not a specialization of any other. The only LHSs we have to check after a
    // specialization are those that are generalizations of the new LHS but not the old one.
    Node* cur_node_ptr = &root_;
    DecisionBoundary& cur_node_rhs_bound = cur_node_ptr->rhs_bounds[rhs_index];
    if (cur_node_rhs_bound >= rhs_bound) return;
    DecisionBoundary* cur_node_rhs_ptr = &cur_node_rhs_bound;

    Index cur_node_index = 0;
    for (Index next_node_index = GetFirstNonZeroIndex(lhs_bounds, cur_node_index);
         next_node_index != column_matches_size_; cur_node_index = next_node_index + 1,
               next_node_index = GetFirstNonZeroIndex(lhs_bounds, cur_node_index)) {
        NodeChildren& children = cur_node_ptr->children;
        if (HasLhsGeneralization(*cur_node_ptr, lhs_bounds, rhs_bound, rhs_index, cur_node_index,
                                 next_node_index + 1))
            return;

        std::size_t const child_array_index = next_node_index - cur_node_index;
        auto [boundary_mapping, is_first_arr] = TryEmplaceChild(children, child_array_index);
        DecisionBoundary const next_lhs_bound = lhs_bounds[next_node_index];
        std::size_t const next_child_array_size = column_matches_size_ - next_node_index;
        if (is_first_arr) [[unlikely]] {
            Node& new_node = boundary_mapping
                                     .try_emplace(next_lhs_bound, column_matches_size_,
                                                  next_child_array_size)
                                     .first->second;
            AddUnchecked(new_node, lhs_bounds, rhs_bound, rhs_index, next_node_index + 1);
            goto succeeded;
        }

        auto it = boundary_mapping.begin();
        for (auto end = boundary_mapping.end(); it != end; ++it) {
            auto& [generalization_bound, node] = *it;
            if (generalization_bound > next_lhs_bound) {
                break;
            }
            if (generalization_bound == next_lhs_bound) {
                goto advance;
            }
            if (HasGeneralization(node, lhs_bounds, rhs_bound, rhs_index, next_node_index + 1)) {
                return;
            }
        }
        {
            Node& new_node = boundary_mapping
                                     .emplace_hint(it, std::piecewise_construct,
                                                   std::forward_as_tuple(next_lhs_bound),
                                                   std::forward_as_tuple(column_matches_size_,
                                                                         next_child_array_size))
                                     ->second;
            // first in bound map
            AddUnchecked(new_node, lhs_bounds, rhs_bound, rhs_index, next_node_index + 1);
        }
        goto succeeded;

    advance:
        cur_node_ptr = &it->second;
        DecisionBoundary& cur_node_rhs_bound = cur_node_ptr->rhs_bounds[rhs_index];
        if (cur_node_rhs_bound >= rhs_bound) return;
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

succeeded:
    std::size_t level = 0;
    for (Index i = 0; i < column_matches_size_; ++i) {
        DecisionBoundary cur_bound = lhs_bounds[i];
        if (cur_bound == kLowestBound) continue;
        level += get_single_level_(cur_bound, i);
    }
    if (level > max_level_) max_level_ = level;
}

bool MdLattice::HasGeneralization(Node const& node, DecisionBoundaryVector const& lhs_bounds,
                                  DecisionBoundary rhs_bound, Index rhs_index,
                                  Index const cur_node_index) const {
    if (node.rhs_bounds[rhs_index] >= rhs_bound) return true;
    if (HasLhsGeneralization(node, lhs_bounds, rhs_bound, rhs_index, cur_node_index,
                             cur_node_index))
        return true;
    return false;
}

bool MdLattice::HasGeneralization(DecisionBoundaryVector const& lhs_bounds,
                                  DecisionBoundary const rhs_bound, Index rhs_index) const {
    return HasGeneralization(root_, lhs_bounds, rhs_bound, rhs_index, 0);
}

void MdLattice::FindViolatedInternal(Node& cur_node, std::vector<MdLatticeNodeInfo>& found,
                                     DecisionBoundaryVector& cur_node_lhs_bounds,
                                     SimilarityVector const& similarity_vector,
                                     Index this_node_index) {
    DecisionBoundaryVector& rhs_bounds = cur_node.rhs_bounds;
    NodeChildren& children = cur_node.children;
    {
        assert(rhs_bounds.size() == similarity_vector.size());
        auto it_rhs = rhs_bounds.begin();
        auto it_sim = similarity_vector.begin();
        auto end_rhs = rhs_bounds.end();
        for (; it_rhs != end_rhs; ++it_rhs, ++it_sim) {
            if (*it_sim < *it_rhs) {
                found.emplace_back(cur_node_lhs_bounds, &rhs_bounds);
                break;
            }
        }
    }
    std::size_t const child_array_size = children.size();
    for (Index child_array_index = FindFirstNonEmptyIndex(children, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(children, child_array_index + 1)) {
        Index const next_node_index = this_node_index + child_array_index;
        DecisionBoundary& cur_lhs_bound = cur_node_lhs_bounds[next_node_index];
        DecisionBoundary const sim_vec_sim = similarity_vector[next_node_index];
        for (auto& [generalization_bound, node] : *children[child_array_index]) {
            if (generalization_bound > sim_vec_sim) break;
            cur_lhs_bound = generalization_bound;
            FindViolatedInternal(node, found, cur_node_lhs_bounds, similarity_vector,
                                 next_node_index + 1);
        }
        cur_lhs_bound = kLowestBound;
    }
}

std::vector<MdLatticeNodeInfo> MdLattice::FindViolated(SimilarityVector const& similarity_vector) {
    std::vector<MdLatticeNodeInfo> found;
    DecisionBoundaryVector current_lhs(similarity_vector.size(), kLowestBound);
    FindViolatedInternal(root_, found, current_lhs, similarity_vector, 0);
    return found;
}

void MdLattice::RaiseInterestingnessBounds(
        Node const& cur_node, DecisionBoundaryVector const& lhs_bounds,
        std::vector<DecisionBoundary>& cur_interestingness_bounds, Index this_node_index,
        std::vector<Index> const& indices) const {
    {
        std::size_t const indices_size = indices.size();
        for (Index i = 0; i < indices_size; ++i) {
            DecisionBoundary const this_node_bound = cur_node.rhs_bounds[indices[i]];
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
        OptionalChild const& optional_child = cur_node.children[child_array_index];
        if (!optional_child.has_value()) continue;
        DecisionBoundary const generalization_bound_limit = lhs_bounds[next_node_index];
        for (auto const& [generalization_bound, node] : *optional_child) {
            if (generalization_bound > generalization_bound_limit) break;
            RaiseInterestingnessBounds(node, lhs_bounds, cur_interestingness_bounds,
                                       next_node_index + 1, indices);
        }
    }
}

std::vector<DecisionBoundary> MdLattice::GetRhsInterestingnessBounds(
        DecisionBoundaryVector const& lhs_bounds, std::vector<Index> const& indices) const {
    std::vector<DecisionBoundary> interestingness_bounds;
    interestingness_bounds.reserve(indices.size());
    for (Index index : indices) {
        DecisionBoundary const lhs_bound = lhs_bounds[index];
        assert(lhs_bound != 1.0);
        interestingness_bounds.push_back(lhs_bound);
    }
    RaiseInterestingnessBounds(root_, lhs_bounds, interestingness_bounds, 0, indices);
    return interestingness_bounds;
}

void MdLattice::GetLevel(Node& cur_node, std::vector<MdLatticeNodeInfo>& collected,
                         DecisionBoundaryVector& cur_node_lhs_bounds, Index cur_node_index,
                         std::size_t level_left) {
    if (level_left == 0) {
        if (NotEmpty(cur_node.rhs_bounds))
            collected.emplace_back(cur_node_lhs_bounds, &cur_node.rhs_bounds);
        return;
    }
    std::size_t const child_array_size = cur_node.children.size();
    for (Index child_array_index = FindFirstNonEmptyIndex(cur_node.children, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(cur_node.children, child_array_index + 1)) {
        Index const next_node_index = cur_node_index + child_array_index;
        DecisionBoundary& next_lhs_bound = cur_node_lhs_bounds[next_node_index];
        for (auto& [boundary, node] : *cur_node.children[child_array_index]) {
            assert(boundary > kLowestBound);
            std::size_t const single = get_single_level_(next_node_index, boundary);
            if (single > level_left) break;
            next_lhs_bound = boundary;
            GetLevel(node, collected, cur_node_lhs_bounds, next_node_index + 1,
                     level_left - single);
        }
        next_lhs_bound = kLowestBound;
    }
}

std::vector<MdLatticeNodeInfo> MdLattice::GetLevel(std::size_t level) {
    std::vector<MdLatticeNodeInfo> collected;
    DecisionBoundaryVector current_lhs(column_matches_size_, kLowestBound);
    GetLevel(root_, collected, current_lhs, 0, level);
    return collected;
}

void MdLattice::GetAll(Node& cur_node, std::vector<MdLatticeNodeInfo>& collected,
                       DecisionBoundaryVector& this_node_lhs_bounds, Index this_node_index) {
    NodeChildren& children = cur_node.children;
    DecisionBoundaryVector& rhs_bounds = cur_node.rhs_bounds;
    if (NotEmpty(rhs_bounds)) collected.emplace_back(this_node_lhs_bounds, &rhs_bounds);
    std::size_t const child_array_size = children.size();
    for (Index child_array_index = FindFirstNonEmptyIndex(children, 0);
         child_array_index != child_array_size;
         child_array_index = FindFirstNonEmptyIndex(children, child_array_index + 1)) {
        Index const next_node_index = this_node_index + child_array_index;
        DecisionBoundary& next_lhs_bound = this_node_lhs_bounds[next_node_index];
        for (auto& [boundary, node] : *children[child_array_index]) {
            next_lhs_bound = boundary;
            GetAll(node, collected, this_node_lhs_bounds, next_node_index + 1);
        }
        next_lhs_bound = kLowestBound;
    }
}

std::vector<MdLatticeNodeInfo> MdLattice::GetAll() {
    std::vector<MdLatticeNodeInfo> collected;
    DecisionBoundaryVector current_lhs(column_matches_size_, kLowestBound);
    GetAll(root_, collected, current_lhs, 0);
    return collected;
}

}  // namespace algos::hymd::lattice
