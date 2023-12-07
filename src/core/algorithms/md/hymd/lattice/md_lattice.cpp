#include "algorithms/md/hymd/lattice/md_lattice.h"

#include <algorithm>
#include <cassert>

namespace algos::hymd::lattice {

MdLattice::MdLattice(size_t column_matches_size, SingleLevelFunc single_level_func)
    : root_(DecisionBoundaryVector(column_matches_size, 1.0)),
      column_matches_size_(column_matches_size),
      get_single_level_(std::move(single_level_func)) {}

size_t MdLattice::GetMaxLevel() const {
    return max_level_;
}

void MdLattice::AddIfMinimal(DecisionBoundaryVector const& lhs_sims,
                             model::md::DecisionBoundary const rhs_sim, model::Index rhs_index) {
    bool succeded = root_.AddIfMinimal(lhs_sims, rhs_sim, rhs_index, 0);
    if (!succeded) return;
    std::size_t level = 0;
    for (size_t i = 0; i < column_matches_size_; ++i) {
        model::md::DecisionBoundary cur_bound = lhs_sims[i];
        if (cur_bound == 0.0) continue;
        level += get_single_level_(cur_bound, i);
    }
    if (level > max_level_) max_level_ = level;
}

bool MdLattice::HasGeneralization(DecisionBoundaryVector const& lhs_sims,
                                  model::md::DecisionBoundary const rhs_sim,
                                  model::Index rhs_index) const {
    return root_.HasGeneralization(lhs_sims, rhs_sim, rhs_index, 0);
}

std::vector<MdLatticeNodeInfo> MdLattice::FindViolated(SimilarityVector const& similarity_vector) {
    std::vector<MdLatticeNodeInfo> found;
    DecisionBoundaryVector lhs(similarity_vector.size(), 0.0);
    root_.FindViolated(found, lhs, similarity_vector, 0);
    return found;
}

std::vector<model::md::DecisionBoundary> MdLattice::GetRhsInterestingnessBounds(
        DecisionBoundaryVector const& lhs, std::vector<model::Index> const& indices) const {
    std::vector<model::md::DecisionBoundary> rhs;
    rhs.reserve(indices.size());
    for (model::Index index : indices) {
        model::md::DecisionBoundary const lhs_bound = lhs[index];
        assert(lhs_bound != 1.0);
        rhs.push_back(lhs_bound);
    }
    root_.RaiseInterestingnessBounds(lhs, rhs, 0, indices);
    return rhs;
}

std::vector<MdLatticeNodeInfo> MdLattice::GetLevel(size_t level) {
    std::vector<MdLatticeNodeInfo> collected;
    DecisionBoundaryVector lhs(column_matches_size_, 0.0);
    root_.GetLevel(collected, lhs, 0, level, get_single_level_);
    return collected;
}

}  // namespace algos::hymd::lattice
