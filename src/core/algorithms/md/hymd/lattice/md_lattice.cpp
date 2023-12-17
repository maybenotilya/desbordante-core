#include "algorithms/md/hymd/lattice/md_lattice.h"

#include <algorithm>
#include <cassert>

namespace algos::hymd::lattice {

// TODO: remove recursion
MdLattice::MdLattice(std::size_t column_matches_size, SingleLevelFunc single_level_func)
    : root_(DecisionBoundaryVector(column_matches_size, 1.0)),
      column_matches_size_(column_matches_size),
      get_single_level_(std::move(single_level_func)) {}

std::size_t MdLattice::GetMaxLevel() const {
    return max_level_;
}

void MdLattice::AddIfMinimal(DecisionBoundaryVector const& lhs_bounds,
                             model::md::DecisionBoundary const rhs_bound, model::Index rhs_index) {
    // TODO: use info about where the LHS was specialized from to reduce generalization checks.
    // When an MD is inferred from, it is not a specialization of any other MD in the lattice, so
    // its LHS is not a specialization of any other. The only LHSs we have to check after a
    // specialization are those that are generalizations of the new LHS but not the old one.
    bool succeded = root_.AddIfMinimal(lhs_bounds, rhs_bound, rhs_index, 0);
    if (!succeded) return;
    std::size_t level = 0;
    for (model::Index i = 0; i < column_matches_size_; ++i) {
        model::md::DecisionBoundary cur_bound = lhs_bounds[i];
        if (cur_bound == 0.0) continue;
        level += get_single_level_(cur_bound, i);
    }
    if (level > max_level_) max_level_ = level;
}

bool MdLattice::HasGeneralization(DecisionBoundaryVector const& lhs_bounds,
                                  model::md::DecisionBoundary const rhs_bound,
                                  model::Index rhs_index) const {
    return root_.HasGeneralization(lhs_bounds, rhs_bound, rhs_index, 0);
}

std::vector<MdLatticeNodeInfo> MdLattice::FindViolated(SimilarityVector const& similarity_vector) {
    std::vector<MdLatticeNodeInfo> found;
    DecisionBoundaryVector current_lhs(similarity_vector.size(), 0.0);
    root_.FindViolated(found, current_lhs, similarity_vector, 0);
    return found;
}

std::vector<model::md::DecisionBoundary> MdLattice::GetRhsInterestingnessBounds(
        DecisionBoundaryVector const& lhs_bounds, std::vector<model::Index> const& indices) const {
    std::vector<model::md::DecisionBoundary> interestingness_bounds;
    interestingness_bounds.reserve(indices.size());
    for (model::Index index : indices) {
        model::md::DecisionBoundary const lhs_bound = lhs_bounds[index];
        assert(lhs_bound != 1.0);
        interestingness_bounds.push_back(lhs_bound);
    }
    root_.RaiseInterestingnessBounds(lhs_bounds, interestingness_bounds, 0, indices);
    return interestingness_bounds;
}

std::vector<MdLatticeNodeInfo> MdLattice::GetLevel(std::size_t level) {
    std::vector<MdLatticeNodeInfo> collected;
    DecisionBoundaryVector current_lhs(column_matches_size_, 0.0);
    root_.GetLevel(collected, current_lhs, 0, level, get_single_level_);
    return collected;
}

std::vector<MdLatticeNodeInfo> MdLattice::GetAll() {
    std::vector<MdLatticeNodeInfo> collected;
    DecisionBoundaryVector current_lhs(column_matches_size_, 0.0);
    root_.GetAll(collected, current_lhs);
    return collected;
}

}  // namespace algos::hymd::lattice
