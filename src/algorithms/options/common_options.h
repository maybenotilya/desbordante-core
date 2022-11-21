#pragma once

#include "names.h"
#include "descriptions.h"
#include "option_type.h"

namespace algos::config {

using EqNullsType = bool;
using NullDIstInfType = bool;

const OptionType<EqNullsType> EqualNulls{{names::kEqualNulls, descriptions::kDEqualNulls}, true};
const OptionType<NullDIstInfType> NullDistInf{{names::kDistToNullIsInfinity,
                                               descriptions::kDDistToNullIsInfinity}, false};

}  // namespace algos::config
