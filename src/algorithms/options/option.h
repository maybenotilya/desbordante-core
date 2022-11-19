#pragma once

#include <cassert>
#include <functional>
#include <optional>

#include "info.h"
#include "ioption.h"

namespace algos::config {
template <typename T>
struct Option : public IOption {
private:
    using OptCondVector =
            std::vector<std::pair<std::function<bool(T const& val)>, std::vector<std::string>>>;
    using OptAddFunc =
            std::function<void(std::string_view const&, std::vector<std::string_view> const&)>;

public:
    Option(OptionInfo const info, T* value_ptr, std::function<void(T&)> value_check,
           std::optional<T> default_value)
        : info_(info),
          value_ptr_(value_ptr),
          value_check_(value_check),
          default_value_(default_value) {}

    void SetDefault() override {
        if (!default_value_.has_value())
            throw std::logic_error(
                    std::string("No value was provided to an option without a default value (") +
                    info_.GetName().data() + ")");
        Set(default_value_.value());
    }

    void Set(T value) {
        if (is_set_) Unset();

        if (value_check_) value_check_(value);
        if (instance_check_) instance_check_(value);

        assert(value_ptr_ != nullptr);
        *value_ptr_ = value;
        is_set_ = true;
        if (opt_add_func_) {
            for (auto const& [cond, opts] : opt_cond_) {
                assert(cond);
                if (cond(value)) {
                    opt_add_func_(info_.GetName(), opts);
                    break;
                }
            }
        }
    }

    void Unset() override {
        is_set_ = false;
    }

    void SetAny(boost::any value) override {
        Set(boost::any_cast<T>(value));
    }

    [[nodiscard]] std::string_view GetName() const override {
        return info_.GetName();
    }

    [[nodiscard]] std::string_view GetDescription() const {
        return info_.GetDescription();
    }

    [[nodiscard]] bool IsSet() const override {
        return is_set_;
    }

    Option& SetInstanceCheck(std::function<void(T&)> instance_check) {
        instance_check_ = instance_check;
        return *this;
    }

    Option& SetConditionalOpts(OptAddFunc const& add_opts, OptCondVector opt_cond) {
        assert(add_opts);
        assert(!opt_cond.empty());
        opt_cond_ = opt_cond;
        opt_add_func_ = add_opts;
        return *this;
    }

private:
    bool is_set_ = false;
    OptionInfo const info_;
    std::optional<T> default_value_{};
    std::function<void(T&)> value_check_{};
    std::function<void(T&)> instance_check_{};
    T* value_ptr_;
    OptCondVector opt_cond_{};
    OptAddFunc opt_add_func_{};
};
}  // namespace algos::config
