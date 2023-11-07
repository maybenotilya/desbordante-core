#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "algorithms/md/hymd/model/full_lattice.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_node.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/validation_info.h"
#include "algorithms/md/hymd/types.h"

namespace algos::hymd::model {

class MinPickerLattice {
private:
    FullLattice* const lattice_;
    size_t const attribute_num_;
    std::unordered_map<SimilarityVector, std::unordered_set<size_t>> picked_;
    size_t cardinality = 0;
    MinPickerNode root_;

    std::vector<ValidationInfo*> GetAll();
    void PickMinimalMds(std::vector<MdLatticeNodeInfo>& mds);
    void Advance();

public:
    std::vector<ValidationInfo*> GetUncheckedLevelMds(std::vector<MdLatticeNodeInfo>& mds);

    explicit MinPickerLattice(FullLattice* const lattice, size_t attribute_num)
        : lattice_(lattice), attribute_num_(attribute_num) {}
};

}  // namespace algos::hymd::model
