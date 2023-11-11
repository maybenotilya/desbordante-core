#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace algos::hymd::preprocessing {
using SimilarityFunction =
        std::function<std::unique_ptr<std::byte const>(std::byte const*, std::byte const*)>;
}  // namespace algos::hymd::preprocessing
