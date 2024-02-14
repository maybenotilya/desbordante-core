#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/md/hymd/lattice/lattice_child_array.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "model/index.h"

namespace algos::hymd::lattice::cardinality {

class MinPickerNode {
private:
    LatticeChildArray<MinPickerNode> children_;
    ValidationInfo* task_info_ = nullptr;

    void AddUnchecked(ValidationInfo* validation_info, model::Index this_node_index);

public:
    void ExcludeGeneralizationRhs(MdLatticeNodeInfo const& lattice_node_info,
                                  model::Index this_node_index,
                                  boost::dynamic_bitset<>& considered_indices);
    void RemoveSpecializations(MdLatticeNodeInfo const& lattice_node_info,
                               model::Index this_node_index,
                               boost::dynamic_bitset<> const& picked_indices);
    void Add(ValidationInfo* validation_info, model::Index this_node_index);
    void GetAll(std::vector<ValidationInfo>& collected, model::Index this_node_index);
    void Clear();

    MinPickerNode(std::size_t children_number);
};

}  // namespace algos::hymd::lattice::cardinality
