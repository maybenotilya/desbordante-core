#pragma once

#include <cstddef>

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::util {
inline size_t GetFirstNonZeroIndex(model::SimilarityVector const& similarity_vector, size_t index) {
    while (index < similarity_vector.size()) {
        if (similarity_vector[index] != 0.0) {
            return index;
        }
        ++index;
    }
    return index;
}
}