#pragma once

#include <vector>

#include "util/py_tuple_hash.h"

namespace std {
template <>
struct hash<std::vector<double>> {
    std::size_t operator()(std::vector<double> const& p) const noexcept {
        constexpr bool kUseJavaHash = true;
        if constexpr (kUseJavaHash) {
            int32_t hash = 1;
            for (double element : p) {
                auto bits = std::bit_cast<int64_t>(element);
                hash = 31 * hash + (bits ^ bits >> 32);
            }
            return hash;
        } else {
            util::PyTupleHash<double> hasher{p.size()};
            for (double el : p) {
                hasher.AddValue(el);
            }
            return hasher.GetResult();
        }
    }
};
}  // namespace std
