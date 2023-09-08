#pragma once

#include <cstddef>

namespace algos::hymd::model {

class Lattice {
public:
    size_t GetMaxLevel();
    void GetLevel(size_t level);
    void GetLowerBoundaries();
    void Validate();

    void Add();
    void AddIfMin();
    void FindViolated();

    void RemoveMd();
    void RemoveNode();
};

}  // namespace algos::hymd::model
