#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "model/types/type.h"

namespace algos::hymd::model {

struct ColumnMatchInternal {
    using ValueComparisonFunction = std::function<double(::model::Type const&, std::byte const*,
                                                         ::model::Type const&, std::byte const*)>;

    size_t left_col_index;
    size_t right_col_index;
    ValueComparisonFunction similarity_function_;

    ColumnMatchInternal(size_t left_col_index, size_t right_col_index,
                        ValueComparisonFunction similarity_function_)
        : left_col_index(left_col_index),
          right_col_index(right_col_index),
          similarity_function_(std::move(similarity_function_)) {}
};

}  // namespace algos::hymd::model