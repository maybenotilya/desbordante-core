#pragma once

#include <vector>

#include "util/py_tuple_hash.h"

namespace std {
template <>
struct hash<std::vector<double>> {
    std::size_t operator()(std::vector<double> const& p) const noexcept {
        util::PyTupleHash<double> hasher{p.size()};
        for (double el : p) {
            hasher.AddValue(el);
        }
        return hasher.GetResult();
    }
};
}  // namespace std
