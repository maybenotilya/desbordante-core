#pragma once

#include "algorithms/md/hymd/types.h"

#include <unordered_set>

namespace algos::hymd::model {

template <typename T>
struct ValueInfo {
    std::vector<T> const data;
    std::unordered_set<ValueIdentifier> const nulls;
    std::unordered_set<ValueIdentifier> const empty;

    ValueInfo(std::vector<T> data, std::unordered_set<ValueIdentifier> nulls,
              std::unordered_set<ValueIdentifier> empty)
        : data(std::move(data)), nulls(std::move(nulls)), empty(std::move(empty)) {}
};

}
