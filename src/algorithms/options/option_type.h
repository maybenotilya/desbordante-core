#pragma once

#include "option.h"

namespace algos::config {

template <typename T>
struct OptionType {
    explicit OptionType(OptionInfo const info, boost::optional<T> default_value = {},
                        std::function<void(T &)> value_check = {}) : info_(info),
          default_value_(std::move(default_value)), value_check_(std::move(value_check)) {}

    OptionType(OptionInfo const info, std::function<void(T &)> value_check)
        : OptionType(info, {}, value_check) {}

    [[nodiscard]] Option<T> GetOption(T* value_ptr) const {
        return {info_, value_ptr, value_check_, default_value_};
    }

private:
    boost::optional<T> const default_value_;
    OptionInfo const info_;
    std::function<void(T&)> const value_check_;
};

}
