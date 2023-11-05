#pragma once

#include <cstddef>
#include <unordered_set>

#include "algorithms/md/hymd/model/md_lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_node.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/types.h"

namespace algos::hymd::model {

class MinPickerLattice {
private:
    DecBoundVectorUnorderedSet picked_lhs_;
    size_t attribute_num_;
    size_t cardinality = 0;
    MinPickerNode root_;

public:
    void PickMinimalMds(std::vector<MdLatticeNodeInfo>& mds);

    void Advance();
    std::vector<MdLatticeNodeInfo> GetAll();

    explicit MinPickerLattice(size_t attribute_num) : attribute_num_(attribute_num) {}
};

}  // namespace algos::hymd::model
