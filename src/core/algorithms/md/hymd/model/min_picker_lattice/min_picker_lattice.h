#pragma once

#include <cstddef>
#include <unordered_set>

#include "algorithms/md/hymd/model/md_lattice/lattice_node_sims.h"
#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_node.h"
#include "algorithms/md/hymd/model/similarity.h"

namespace std {
template <>
struct hash<algos::hymd::model::SimilarityVector> {
    std::size_t operator()(algos::hymd::model::SimilarityVector const& p) const {
        auto hasher = std::hash<double>{};
        std::size_t hash = 0;
        for (double el : p) {
            hash ^= hasher(el);
        }
        return hash;
    }
};
}  // namespace std

namespace algos::hymd::model {

class MinPickerLattice {
private:
    std::unordered_set<SimilarityVector> picked_lhs_;
    size_t attribute_num_;
    MinPickerNode root_;

public:
    void PickMinimalMds(std::vector<LatticeNodeSims> const& mds);

    void Advance();
    std::vector<LatticeNodeSims> GetAll();

    explicit MinPickerLattice(size_t attribute_num) : attribute_num_(attribute_num) {}
};

}  // namespace algos::hymd::model