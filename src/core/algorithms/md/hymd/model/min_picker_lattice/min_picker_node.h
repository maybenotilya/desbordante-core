#include <cstddef>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/model/md_lattice/lattice_node_sims.h"
#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class MinPickerNode {
private:
    using ThresholdMap = std::map<double, std::unique_ptr<MinPickerNode>>;

    SimilarityVector rhs_;
    std::unordered_map<size_t, ThresholdMap> children_;

public:
    void Add(LatticeNodeSims const& md, size_t this_node_index);
    bool IsMd() {
        return !rhs_.empty();
    };
    bool HasGeneralization(LatticeNodeSims const& md, size_t this_node_index);
    void TryAdd(LatticeNodeSims const& md);
    void RemoveSpecializations(LatticeNodeSims const& md, size_t this_node_index);

    void GetAll(std::vector<LatticeNodeSims>& collected, SimilarityVector& this_node_lhs,
                size_t this_node_index, size_t sims_left);
};

}  // namespace algos::hymd::model