#pragma once

#include <thread>

#include "option_descriptions.h"
#include "option_names.h"
#include "option_type.h"

namespace algos::config {

using EqNullsType = bool;
using NullDIstInfType = bool;
using ThreadNumType = ushort;

const OptionType<EqNullsType> EqualNulls{{names::kEqualNulls, descriptions::kDEqualNulls}, true};
const OptionType<NullDIstInfType> NullDistInf{
        {names::kDistToNullIsInfinity, descriptions::kDDistToNullIsInfinity}, false};
const OptionType<ThreadNumType> ThreadNumber{
        {names::kThreads, descriptions::kDThreads}, 0, [](auto& value) {
            if (value == 0) {
                value = std::thread::hardware_concurrency();
                if (value == 0) {
                    throw std::runtime_error(
                            "Unable to detect number of concurrent "
                            "threads supported by your system. "
                            "Please, specify it manually.");
                }
            }
        }};

}  // namespace algos::config
