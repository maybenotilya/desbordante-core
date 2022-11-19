#pragma once

#include <memory>
#include <string_view>

#include "boost/any.hpp"

namespace algos::config {

struct IOption {
    virtual void Set(boost::optional<boost::any> value_holder) = 0;
    virtual void Unset() = 0;
    [[nodiscard]] virtual bool IsSet() const = 0;
    [[nodiscard]] virtual std::string_view GetName() const = 0;
    virtual ~IOption() = default;
};

}  // namespace algos::config

