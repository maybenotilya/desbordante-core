#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/lattice/lattice_child_array.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "model/index.h"

namespace algos::hymd::lattice::cardinality {

class MinPickerNode {
private:
    ValidationInfo* task_info_ = nullptr;
    LatticeChildArray<MinPickerNode> children_;

    void AddUnchecked(ValidationInfo* info, model::Index this_node_index);

public:
    void ExcludeGeneralizationRhs(MdLatticeNodeInfo const& md, model::Index this_node_index,
                                  std::unordered_set<model::Index>& considered_indices);
    void RemoveSpecializations(MdLatticeNodeInfo const& md, model::Index this_node_index,
                               std::unordered_set<model::Index> const& indices);
    void Add(ValidationInfo* info, model::Index this_node_index);
};

}  // namespace algos::hymd::lattice::cardinality
