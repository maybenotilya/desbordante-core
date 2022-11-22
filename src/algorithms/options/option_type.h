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

    [[nodiscard]] std::string_view GetName() const {
        return info_.GetName();
    }

private:
    OptionInfo const info_;
    boost::optional<T> const default_value_;
    std::function<void(T&)> const value_check_;
};

template <typename T, typename... Options>
void AddNames(std::vector<std::string_view>& names, OptionType<T> opt, Options... options) {
    names.emplace_back(opt.GetName());
    if constexpr (sizeof...(options) != 0) {
        AddNames(names, options...);
    }
}

template <typename T, typename... Options>
std::vector<std::string_view> GetOptionNames(OptionType<T> opt, Options... options) {
    std::vector<std::string_view> names{};
    AddNames(names, opt, options...);
    return names;
}

}
