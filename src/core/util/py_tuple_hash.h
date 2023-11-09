#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace util {

template <typename T>
class PyTupleHash {
    std::size_t res_ = 0x345678UL;
    std::size_t mult_ = 1000003UL;
    std::hash<T> const hasher{};
    std::int64_t len_;

public:
    PyTupleHash(size_t len) : len_(len) {}

    [[nodiscard]] size_t GetResult() const {
        return res_;
    }

    void AddValue(T const& value) noexcept {
        --len_;
        size_t hash = hasher(value);
        res_ = (res_ ^ hash) * mult_;
        mult_ += 82520UL + len_ + len_;
    }
};

}  // namespace util
