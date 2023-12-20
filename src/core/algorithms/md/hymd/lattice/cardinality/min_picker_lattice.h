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
    MinPickerNode root_;
    std::vector<ValidationInfo> info_;

public:
    static constexpr bool kNeedsEmptyRemoval = false;

    void NewBatch(std::size_t max_elements);
    void AddGeneralizations(MdLatticeNodeInfo& lattice_node_info,
                            boost::dynamic_bitset<>& considered_indices);
    std::vector<ValidationInfo> GetAll() noexcept(kNeedsEmptyRemoval);
};

}  // namespace algos::hymd::lattice::cardinality
