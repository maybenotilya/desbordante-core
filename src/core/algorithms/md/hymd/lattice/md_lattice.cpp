#include "algorithms/md/hymd/lattice/md_lattice.h"

#include <algorithm>

namespace {

size_t GetCardinality(algos::hymd::DecisionBoundaryVector const& lhs) {
    return lhs.size() - std::count(lhs.begin(), lhs.end(), 0.0);
}

}  // namespace

namespace algos::hymd::lattice {

MdLattice::MdLattice(size_t column_matches_size)
    : root_(DecisionBoundaryVector(column_matches_size, 1.0)),
      column_matches_size_(column_matches_size) {}

size_t MdLattice::GetMaxLevel() const {
    return max_level_;
}

void MdLattice::AddIfMinimal(DecisionBoundaryVector const& lhs_sims,
                             model::md::DecisionBoundary const rhs_sim, model::Index rhs_index) {
    root_.AddIfMinimal(lhs_sims, rhs_sim, rhs_index, 0);
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

std::vector<model::md::DecisionBoundary> MdLattice::GetMaxValidGeneralizationRhs(
        DecisionBoundaryVector const& lhs) const {
    std::vector<model::md::DecisionBoundary> rhs = lhs;
    root_.GetMaxValidGeneralizationRhs(lhs, rhs, 0);
    return rhs;
}

std::vector<MdLatticeNodeInfo> MdLattice::GetLevel(size_t level) {
    std::vector<MdLatticeNodeInfo> collected;
    DecisionBoundaryVector lhs(column_matches_size_, 0.0);
    root_.GetLevel(collected, lhs, 0, level, get_single_level_);
    return collected;
}

}  // namespace algos::hymd::lattice
