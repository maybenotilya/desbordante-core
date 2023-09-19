#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace algos::hymd::model {

class KeyedPositionListIndex {
public:
    using Cluster = std::vector<size_t>;

private:
    std::unordered_map<std::string, size_t> value_id_mapping_;
    std::vector<Cluster> clusters_;
    size_t cur_record_id_ = 0;
    size_t next_value_id_ = 0;

public:
    size_t AddNextValue(std::string value);
    std::unordered_map<std::string, size_t> const& GetMapping() const {
        return value_id_mapping_;
    }
    std::vector<Cluster> const& GetClusters() const {
        return clusters_;
    }
};

}  // namespace algos::hymd::model