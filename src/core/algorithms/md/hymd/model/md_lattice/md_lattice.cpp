#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"

#include <algorithm>

namespace {

size_t GetCardinality(algos::hymd::model::SimilarityVector const& lhs) {
    return lhs.size() - std::count(lhs.begin(), lhs.end(), 0.0);
}

}

namespace algos::hymd::model {

MdLattice::MdLattice(size_t column_matches_size)
    : root_(SimilarityVector(column_matches_size, 1.0)),
      column_matches_size_(column_matches_size) {}

size_t MdLattice::GetMaxLevel() const {
    return max_level_;
}

void MdLattice::Add(LatticeMd const& md) {
    root_.Add(md, 0);
    max_level_ = std::max(max_level_, GetCardinality(md.lhs_sims));
}

void MdLattice::AddIfMin(LatticeMd const& md) {
    if (!root_.AddIfMin(md)) return;
    max_level_ = std::max(max_level_, GetCardinality(md.lhs_sims));
}

void MdLattice::RemoveNode(SimilarityVector const& lhs) {
    root_.RemoveNode(lhs, 0);
}

void MdLattice::RemoveMd(LatticeMd const& md) {
    root_.RemoveMd(md, 0);
}

std::vector<LatticeMd> MdLattice::FindViolated(SimilarityVector const& similarity_vector) {
    std::vector<LatticeMd> found;
    SimilarityVector lhs(similarity_vector.size(), 0.0);
    root_.FindViolated(found, lhs, similarity_vector, 0);
    return found;
}

SimilarityVector MdLattice::GetMaxValidGeneralizationRhs(SimilarityVector const& lhs) {
    SimilarityVector rhs(lhs.size(), 0.0);
    root_.GetMaxValidGeneralizationRhs(lhs, rhs, 0);
    return rhs;
}

std::vector<LatticeNodeSims> MdLattice::GetLevel(size_t level) {
    std::vector<LatticeNodeSims> collected;
    SimilarityVector lhs(column_matches_size_, 0.0);
    root_.GetLevel(collected, lhs, 0, level);
    return collected;
}

}  // namespace algos::hymd::model
