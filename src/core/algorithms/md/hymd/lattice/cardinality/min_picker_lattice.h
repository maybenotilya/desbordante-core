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
    size_t const attribute_num_;
    MinPickerNode root_;
    std::vector<ValidationInfo> info_;

public:
    explicit MinPickerLattice(size_t attribute_num) : attribute_num_(attribute_num) {}
    void NewBatch(std::size_t max_elements);
    void AddGeneralizations(MdLatticeNodeInfo& md,
                            std::unordered_set<model::Index>& considered_indices);
    std::vector<ValidationInfo> GetAll();
};

}  // namespace algos::hymd::lattice::cardinality
