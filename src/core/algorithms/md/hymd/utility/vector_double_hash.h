#pragma once

#include <vector>

#include "algorithms/md/hymd/utility/java_hash.h"
#include "util/py_tuple_hash.h"

namespace std {
template <>
struct hash<std::vector<double>> {
    std::size_t operator()(std::vector<double> const& p) const noexcept {
        constexpr bool kUseJavaHash = true;
        if constexpr (kUseJavaHash) {
            return utility::HashIterable(
                    p, [](double element) { return std::bit_cast<int64_t>(element); });
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
