#include <unordered_map>
#include <boost/python/dict.hpp>
#include <boost/any.hpp>

#include "ProgramOptionStrings.h"

#pragma once

namespace python_bindings
{
namespace py = boost::python;

using ParamsMap = std::unordered_map<std::string, boost::any>;

ParamsMap GetParamsMap(const py::dict & dict);

}  // namespace python_bindings
