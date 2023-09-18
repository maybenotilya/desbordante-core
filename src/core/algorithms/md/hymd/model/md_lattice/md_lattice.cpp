#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"

#include <algorithm>

namespace {

size_t GetCardinality(algos::hymd::model::SimilarityVector const& lhs) {
    return std::count(lhs.begin(), lhs.end(), 0.0);
}

}

namespace algos::hymd::model {

MdLattice::MdLattice(size_t column_matches_size)
    : root_(SimilarityVector(column_matches_size, 1.0)) {}

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

}  // namespace algos::hymd::model
