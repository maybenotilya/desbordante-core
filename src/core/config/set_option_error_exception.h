#pragma once

#include "config/set_option_error.h"

namespace config {

class ConfigurationError : std::exception {
    std::string message_;

public:
    explicit ConfigurationError(std::string message) : message_(std::move(message)) {}
};
class UnknownOption : ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class TypeMismatch : ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class NoDefault : ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class MustSpecifyExplicitly : ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class OutOfRange : ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class InvalidChoice : ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class EmptyCollection : ConfigurationError {
    using ConfigurationError::ConfigurationError;
};

void ThrowOnConfigError(SetOptionErrorInfo const& error_type);

}  // namespace config
