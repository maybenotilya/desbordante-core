#pragma once

#include <cstdint>

namespace utility {
#pragma GCC push_options
#pragma GCC optimize("wrapv")
template <typename Iterable>
std::int32_t HashIterable(Iterable const& iterable) {
    return HashIterable(iterable, [](auto v) { return v; });
}

template <typename Iterable, typename Func>
std::int32_t HashIterable(Iterable const& iterable, Func transform) {
    std::int32_t hash = 1;
    for (auto element : iterable) {
        auto value = transform(element);
        hash = 31 * hash + (value ^ value >> 32);
    }
    return hash;
}

template <typename HashIterable>
std::int32_t CombineHashes(HashIterable const& hash_iterable) {
    std::int32_t hash = 1;
    for (std::int32_t hash_part : hash_iterable) {
        hash = (hash * 59) + hash_part;
    }
    return hash;
}
#pragma GCC pop_options
}  // namespace utility