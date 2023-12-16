#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"

#include <cassert>

namespace algos::hymd::lattice::cardinality {

void MinPickerLattice::NewBatch(std::size_t max_elements) {
    info_.clear();
    info_.reserve(max_elements);
    root_ = MinPickerNode();
}

void MinPickerLattice::AddGeneralizations(MdLatticeNodeInfo& lattice_node_info,
                                          boost::dynamic_bitset<>& considered_indices) {
    root_.ExcludeGeneralizationRhs(lattice_node_info, 0, considered_indices);
    if (considered_indices.none()) return;
    root_.RemoveSpecializations(lattice_node_info, 0, considered_indices);
    ValidationInfo& added_ref =
            info_.emplace_back(&lattice_node_info, std::move(considered_indices));
    root_.Add(&added_ref, 0);
}

std::vector<ValidationInfo> MinPickerLattice::GetAll() {
    if constexpr (kNeedsEmptyRemoval) {
        // TODO: investigate different orders.
        return std::move(info_);

    } else {
        std::vector<ValidationInfo> collected;
        collected.reserve(info_.size());
        root_.GetAll(collected);
        return collected;
    }
}

}  // namespace algos::hymd::lattice::cardinality
