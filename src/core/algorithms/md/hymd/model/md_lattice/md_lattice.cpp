#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"

#include <algorithm>

namespace {

size_t GetCardinality(algos::hymd::model::SimilarityVector const& lhs) {
    return lhs.size() - std::count(lhs.begin(), lhs.end(), 0.0);
}

}  // namespace

namespace algos::hymd::model {

MdLattice::MdLattice(size_t column_matches_size)
    : root_(SimilarityVector(column_matches_size, 1.0)),
      column_matches_size_(column_matches_size) {}

size_t MdLattice::GetMaxLevel() const {
    return max_level_;
}

void MdLattice::Add(SimilarityVector const& lhs_sims, Similarity rhs_sim, size_t rhs_index) {
    max_level_ = std::max(max_level_, GetCardinality(lhs_sims));
    root_.Add(lhs_sims, rhs_sim, rhs_index, 0);
}

void MdLattice::Add(LatticeMd const& md) {
    max_level_ = std::max(max_level_, GetCardinality(md.lhs_sims));
    root_.Add(md.lhs_sims, md.rhs_sim, md.rhs_index, 0);
}

void MdLattice::AddIfMin(SimilarityVector const& lhs_sims, Similarity rhs_sim, size_t rhs_index) {
    if (HasGeneralization(lhs_sims, rhs_sim, rhs_index)) return;
    Add(lhs_sims, rhs_sim, rhs_index);
}

void MdLattice::RemoveNode(SimilarityVector const& lhs) {
    root_.RemoveNode(lhs, 0);
}

bool MdLattice::HasGeneralization(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                  size_t rhs_index) const {
    return root_.HasGeneralization(lhs_sims, rhs_sim, rhs_index, 0);
}

std::vector<MdLatticeNodeInfo> MdLattice::FindViolated(SimilarityVector const& similarity_vector) {
    std::vector<MdLatticeNodeInfo> found;
    SimilarityVector lhs(similarity_vector.size(), 0.0);
    root_.FindViolated(found, lhs, similarity_vector, 0);
    return found;
}

SimilarityVector MdLattice::GetMaxValidGeneralizationRhs(SimilarityVector const& lhs) const {
    SimilarityVector rhs = lhs;
    root_.GetMaxValidGeneralizationRhs(lhs, rhs, 0);
    return rhs;
}

std::vector<LatticeNodeSims> MdLattice::GetLevel(size_t level) const {
    std::vector<LatticeNodeSims> collected;
    SimilarityVector lhs(column_matches_size_, 0.0);
    root_.GetLevel(collected, lhs, 0, level);
    return collected;
}

}  // namespace algos::hymd::model
