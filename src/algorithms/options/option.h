#pragma once

#include <cassert>
#include <functional>
#include <optional>

#include "boost/any.hpp"
#include "boost/optional.hpp"
#include "ioption.h"
#include "option_info.h"

namespace algos::config {
template <typename T>
struct Option : public IOption {
private:
    using OptCondVector =
            std::vector<std::pair<std::function<bool(T const& val)>, std::vector<std::string_view>>>;
    using OptAddFunc =
            std::function<void(std::string_view const&, std::vector<std::string_view> const&)>;

public:
    Option(OptionInfo const info, T* value_ptr, std::function<void(T&)> normalize,
           boost::optional<T> default_value)
        : info_(info),
          value_ptr_(value_ptr),
          normalize_(normalize),
          default_func_(default_value.has_value()
                                ? [default_value]() { return default_value.value(); }
                                : std::function<T()>{})
    {}

    void Set(boost::optional<boost::any> value_holder) override {
        assert(!is_set_);
        T value = GetValue(value_holder);
        if (normalize_) normalize_(value);
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

    T GetValue(boost::optional<boost::any> value_holder) const {
        std::string const no_value_no_default =
                std::string("No value was provided to an option without a default value (")
                + info_.GetName().data() + ")";
        if (!value_holder.has_value()) {
            if (!default_func_) throw std::logic_error(no_value_no_default);
            return default_func_();
        } else {
            return boost::any_cast<T>(value_holder.value());
        }
    }

    void Unset() override {
        is_set_ = false;
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

    Option& OverrideDefaultValue(boost::optional<T> new_default) {
        default_func_ = new_default.has_value()
                                ? [new_default]() { return new_default.value(); }
                                : std::function<T()>{};
        return *this;
    }

    Option& OverrideDefaultFunction(std::function<T()> default_func) {
        default_func_ = default_func;
        return *this;
    }

private:
    bool is_set_ = false;
    OptionInfo const info_;
    T* value_ptr_;
    std::function<void(T&)> normalize_{};
    std::function<T()> default_func_;
    std::function<void(T&)> instance_check_{};
    OptCondVector opt_cond_{};
    OptAddFunc opt_add_func_{};
};

}  // namespace algos::config
