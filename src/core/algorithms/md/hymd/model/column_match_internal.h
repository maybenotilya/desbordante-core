#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace algos::hymd::model {

struct ColumnMatchInternal {
    size_t left_col_index;
    size_t right_col_index;
    std::function<double(std::string, std::string)> similarity_function_;

    ColumnMatchInternal(size_t left_col_index, size_t right_col_index,
                        std::function<double(std::string, std::string)> similarity_function_)
        : left_col_index(left_col_index),
          right_col_index(right_col_index),
          similarity_function_(std::move(similarity_function_)) {}
};

}  // namespace algos::hymd::model