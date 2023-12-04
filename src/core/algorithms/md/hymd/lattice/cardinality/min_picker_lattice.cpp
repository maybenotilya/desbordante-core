#include "algorithms/md/hymd/lattice/cardinality/min_picker_lattice.h"

#include <cassert>

namespace algos::hymd::lattice::cardinality {

void MinPickerLattice::NewBatch(std::size_t max_elements) {
    info_.clear();
    info_.reserve(max_elements);
    root_ = MinPickerNode();
}

void MinPickerLattice::AddGeneralizations(MdLatticeNodeInfo& md,
                                          std::unordered_set<model::Index>& considered_indices) {
    root_.ExcludeGeneralizationRhs(md, 0, considered_indices);
    if (considered_indices.empty()) return;
    root_.RemoveSpecializations(md, 0, considered_indices);
    ValidationInfo& ref = info_.emplace_back(&md, std::move(considered_indices));
    root_.Add(&ref, 0);
}

std::vector<ValidationInfo> MinPickerLattice::GetAll() {
    if constexpr (kNeedsEmptyRemoval) {
        // This actually gives a worse ordering and increases runtime (8.1s vs 15.0s on adult.csv
        // with no parallelism, hybrid).
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
