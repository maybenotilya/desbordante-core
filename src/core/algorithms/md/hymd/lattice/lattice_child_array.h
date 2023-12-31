#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <unordered_map>

#include "algorithms/md/decision_boundary.h"
#include "model/index.h"

namespace algos::hymd::lattice {
template <typename NodeType>
using BoundaryMap = std::map<model::md::DecisionBoundary, NodeType>;

template <typename NodeType>
using LatticeChildArray = std::vector<std::optional<BoundaryMap<NodeType>>>;

template <typename NodeType>
std::size_t FindFirstNonEmptyIndex(LatticeChildArray<NodeType> const& child_array,
                                   model::Index index) {
    for (std::size_t const array_size = child_array.size(); index < array_size; ++index) {
        if (child_array[index].has_value()) return index;
    }
    return index;
}

template <typename NodeType, typename... Args>
std::pair<BoundaryMap<NodeType>&, bool> TryEmplaceChild(LatticeChildArray<NodeType>& child_array,
                                                        model::Index index, Args&&... args) {
    std::optional<BoundaryMap<NodeType>>& optional_child = child_array[index];
    if (optional_child.has_value()) {
        return {*optional_child, false};
    }
    return {optional_child.emplace(std::forward<Args>(args)...), true};
}
}  // namespace algos::hymd::lattice
