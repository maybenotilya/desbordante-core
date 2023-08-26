#include "config/set_option_error_exception.h"

#include <functional>
#include <unordered_map>

namespace config {

template <SetOptionErrorType::_integral EnumVal, typename T>
std::pair<SetOptionErrorType, std::function<void(std::string)>> EnumExcPair{
        SetOptionErrorType::_from_integral(EnumVal), [](std::string message) {
            throw T(std::move(message));
        }};

std::unordered_map<SetOptionErrorType, std::function<void(std::string)>> throw_function_map {
        EnumExcPair<SetOptionErrorType::kUnknownOption, UnknownOption>,
        EnumExcPair<SetOptionErrorType::kTypeMismatch, TypeMismatch>,
        EnumExcPair<SetOptionErrorType::kNoDefault, NoDefault>,
        EnumExcPair<SetOptionErrorType::kMustSpecifyExplicitly, MustSpecifyExplicitly>,
        EnumExcPair<SetOptionErrorType::kOutOfRange, OutOfRange>,
        EnumExcPair<SetOptionErrorType::kInvalidChoice, InvalidChoice>,
        EnumExcPair<SetOptionErrorType::kEmptyCollection, EmptyCollection>,
};

void ThrowOnConfigError(SetOptionErrorInfo const& error_info) {
    throw_function_map.at(error_info.error)(error_info.message);
}

}
