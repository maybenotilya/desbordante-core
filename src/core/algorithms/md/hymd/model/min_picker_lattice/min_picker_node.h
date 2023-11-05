#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/model/md_lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class MinPickerNode {
private:
    using ThresholdMap = std::map<double, std::unique_ptr<MinPickerNode>>;

    SimilarityVector* rhs_ = nullptr;
    std::unordered_map<size_t, ThresholdMap> children_;

public:
    void Add(MdLatticeNodeInfo const& md, size_t this_node_index);
    bool IsMd() {
        return rhs_ != nullptr;
    };
    bool HasGeneralization(MdLatticeNodeInfo const& md, size_t this_node_index);
    void TryAdd(MdLatticeNodeInfo const& md);
    void RemoveSpecializations(MdLatticeNodeInfo const& md, size_t this_node_index);

    void GetAll(std::vector<MdLatticeNodeInfo>& collected, SimilarityVector& this_node_lhs,
                size_t this_node_index, size_t sims_left);
};

}  // namespace algos::hymd::model
