#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <unordered_map>

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class SupportNode {
    using ThresholdMap = std::map<double, std::unique_ptr<SupportNode>>;

    bool is_unsupported_ = false;
    std::unordered_map<size_t, ThresholdMap> children_;

public:
    bool IsUnsupported(model::SimilarityVector const& lhs_vec, size_t this_node_index) const;
    void MarkUnsupported(model::SimilarityVector const& lhs_vec, size_t this_node_index);
};

}  // namespace algos::hymd::model