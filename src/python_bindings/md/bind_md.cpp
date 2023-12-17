#include "md/bind_md.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/md/md.h"
#include "algorithms/md/mining_algorithms.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
using algos::hymd::HyMD;
using model::MD;
}  // namespace

namespace python_bindings {
void BindMd(py::module_& main_module) {
    using namespace algos;

    auto md_module = main_module.def_submodule("md");
    py::class_<MD>(md_module, "MD")
            .def_property_readonly("lhs_bounds", &MD::GetLhsDecisionBounds)
            .def_property_readonly("rhs", &MD::GetRhs)
            .def("to_long_string", &MD::ToStringFull)
            .def("to_short_string", &MD::ToStringShort)
            .def("__str__", &MD::ToStringShort);
    BindPrimitive<hymd::HyMD>(md_module, &MdAlgorithm::MdList, "MdAlgorithm", "get_mds", {"HyMD"});
}
}  // namespace python_bindings
