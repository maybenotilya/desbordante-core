#pragma once

#include <cstddef>
#include <string>

namespace model {

struct ColumnMatch {
    std::string left_col_name;
    std::string right_col_name;
    std::string similarity_function_name;

    ColumnMatch(std::string left_col_name, std::string right_col_name,
                std::string similarity_function_name)
        : left_col_name(std::move(left_col_name)),
          right_col_name(std::move(right_col_name)),
          similarity_function_name(std::move(similarity_function_name)) {}
};

}  // namespace model
