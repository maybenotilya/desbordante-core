#pragma once

#include <cstddef>
#include <optional>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/model/md_lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/validation_info.h"

namespace algos::hymd::model {

class MinPickerNode {
private:
    using ThresholdMap = std::map<double, std::unique_ptr<MinPickerNode>>;

    std::optional<ValidationInfo> task_info_;
    std::unordered_map<size_t, ThresholdMap> children_;

public:
    void ExcludeGeneralizationRhs(MdLatticeNodeInfo const& md, size_t this_node_index,
                                  std::unordered_set<size_t>& considered_indices);
    void RemoveSpecializations(MdLatticeNodeInfo const& md, size_t this_node_index,
                               std::unordered_set<size_t> const& indices);
    void Add(MdLatticeNodeInfo& md, size_t this_node_index, std::unordered_set<size_t>& indices);

    void GetAll(std::vector<ValidationInfo*>& collected, size_t sims_left);
};

}  // namespace algos::hymd::model
