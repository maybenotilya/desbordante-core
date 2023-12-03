#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "algorithms/md/hymd/lattice/cardinality/min_picker_node.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "model/index.h"

namespace algos::hymd::lattice::cardinality {

class MinPickerLattice {
private:
    FullLattice* const lattice_;
    size_t const attribute_num_;
    std::unordered_map<DecisionBoundaryVector, std::unordered_set<model::Index>> picked_;
    MinPickerNode root_;

    std::vector<ValidationInfo*> GetAll();
    void PickMinimalMds(std::vector<MdLatticeNodeInfo>& mds);
    void Advance();

public:
    std::vector<ValidationInfo*> GetUncheckedLevelMds(std::vector<MdLatticeNodeInfo>& mds);

    explicit MinPickerLattice(FullLattice* const lattice, size_t attribute_num)
        : lattice_(lattice), attribute_num_(attribute_num) {}
};

}  // namespace algos::hymd::lattice::cardinality
