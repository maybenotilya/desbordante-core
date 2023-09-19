#pragma once

#include <cstddef>
#include <string>

namespace model {

struct ColumnMatch {
    size_t left_col_index;
    size_t right_col_index;
    std::string similarity_function_name;

    ColumnMatch(size_t left_col_index, size_t right_col_index, std::string similarity_function_name)
        : left_col_index(left_col_index),
          right_col_index(right_col_index),
          similarity_function_name(std::move(similarity_function_name)) {}
};

}  // namespace model
