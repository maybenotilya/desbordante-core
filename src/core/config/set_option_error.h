#pragma once

#include <enum.h>

#include <utility>

namespace config {

// clang-format off
BETTER_ENUM(SetOptionErrorType, char,
    kUnknownOption,
    kTypeMismatch,
    kNoDefault, // Option does not have a default value, but value is not specified.
    kMustSpecifyExplicitly, // Value has a default, but it cannot be used.
    kOutOfRange,
    kInvalidChoice,
    kEmptyCollection // Value is a collection that should not be empty but is.
)
// clang-format on

struct SetOptionErrorInfo {
    SetOptionErrorType error = SetOptionErrorType::kUnknownOption;
    std::string message;

    SetOptionErrorInfo(SetOptionErrorType error, std::string message)
        : error(error), message(std::move(message)) {}
};

}  // namespace config

BETTER_ENUMS_DECLARE_STD_HASH(config::SetOptionErrorType)
