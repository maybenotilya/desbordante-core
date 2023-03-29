#include <filesystem>

#include <easylogging++.h>
#include <pybind11/pybind11.h>

#include "algorithms/algorithms.h"
#include "model/ar.h"
#include "py_ar_algorithm.h"
#include "py_data_stats.h"
#include "py_fd_algorithm.h"
#include "py_fd_verifier.h"
#include "py_metric_verifier.h"

INITIALIZE_EASYLOGGINGPP

#define DEFINE_ALGORITHM(type, base) \
    py::class_<Py##type, Py##base##Base>(module, #type).def(py::init<>())
#define DEFINE_ALGORITHM_BASE(base) py::class_<Py##base##Base, PyAlgorithmBase>(module, #base)
#define DEFINE_FD_ALGORITHM(type) DEFINE_ALGORITHM(type, FdAlgorithm)
#define DEFINE_AR_ALGORITHM(type) DEFINE_ALGORITHM(type, ArAlgorithm)

namespace python_bindings {

namespace py = pybind11;
using PyApriori = PyArAlgorithm<algos::Apriori>;
using PyTane = PyFDAlgorithm<algos::Tane>;
using PyPyro = PyFDAlgorithm<algos::Pyro>;
using PyFUN = PyFDAlgorithm<algos::FUN>;
using PyFdMine = PyFDAlgorithm<algos::Fd_mine>;
using PyFastFDs = PyFDAlgorithm<algos::FastFDs>;
using PyHyFD = PyFDAlgorithm<algos::hyfd::HyFD>;
using PyFDep = PyFDAlgorithm<algos::FDep>;
using PyDFD = PyFDAlgorithm<algos::DFD>;
using PyDepminer = PyFDAlgorithm<algos::Depminer>;
using PyAid = PyFDAlgorithm<algos::Aid>;
using FDHighlight = algos::fd_verifier::Highlight;
using model::ARStrings;

PYBIND11_MODULE(desbordante, module) {
    using namespace pybind11::literals;

    if (std::filesystem::exists("logging.conf")) {
        el::Loggers::configureFromGlobal("logging.conf");
    } else {
        el::Configurations conf;
        conf.set(el::Level::Global, el::ConfigurationType::Enabled, "false");
        el::Loggers::reconfigureAllLoggers(conf);
    }

    module.doc() = "A data profiling library";

    py::class_<ARStrings>(module, "AssociativeRule")
            .def("__str__", &ARStrings::ToString)
            .def_readonly("left", &ARStrings::left)
            .def_readonly("right", &ARStrings::right)
            .def_readonly("confidence", &ARStrings::confidence);

    py::class_<PyFD>(module, "FD")
            .def("__str__", &PyFD::ToString)
            .def("__repr__", &PyFD::ToString)
            .def_property_readonly("lhs_indices", &PyFD::GetLhs)
            .def_property_readonly("rhs_index", &PyFD::GetRhs);

    py::class_<FDHighlight>(module, "FDHighlight")
            .def_property_readonly("cluster", &FDHighlight::GetCluster)
            .def_property_readonly("num_distinct_rhs_values", &FDHighlight::GetNumDistinctRhsValues)
            .def_property_readonly("most_frequent_rhs_value_proportion",
                                   &FDHighlight::GetMostFrequentRhsValueProportion);

    py::class_<PyAlgorithmBase>(module, "Algorithm")
            .def("fit",
                 py::overload_cast<std::string const &, char, bool, py::kwargs const &>(
                         &PyAlgorithmBase::Fit),
                 "path"_a, "separator"_a = ',', "has_header"_a = true, "Transform data from CSV")
            .def("fit",
                 py::overload_cast<pybind11::object, std::string, py::kwargs const &>(
                         &PyAlgorithmBase::Fit),
                 "df"_a, "name"_a = "Pandas dataframe", "Transform data from pandas dataframe")
            .def("get_needed_options", &PyAlgorithmBase::GetNeededOptions,
                 "Get names of options the algorithm needs")
            .def("set_option", &PyAlgorithmBase::SetOption, "option_name"_a,
                 "option_value"_a = pybind11::none(), "Set option value")
            .def("execute", &PyAlgorithmBase::Execute, "Process data");

    DEFINE_ALGORITHM_BASE(ArAlgorithm).def("get_ars", &PyArAlgorithmBase::GetARs);
    DEFINE_ALGORITHM_BASE(FdAlgorithm).def("get_fds", &PyFdAlgorithmBase::GetFDs);

    DEFINE_ALGORITHM(FDVerifier, Algorithm)
            .def("fd_holds", &PyFDVerifier::FDHolds)
            .def("get_error", &PyFDVerifier::GetError)
            .def("get_num_error_clusters", &PyFDVerifier::GetNumErrorClusters)
            .def("get_num_error_rows", &PyFDVerifier::GetNumErrorRows)
            .def("get_highlights", &PyFDVerifier::GetHighlights);

    DEFINE_ALGORITHM(DataStats, Algorithm).def("get_result_string", &PyDataStats::GetResultString);

    DEFINE_ALGORITHM(MetricVerifier, Algorithm).def("mfd_holds", &PyMetricVerifier::MfdHolds);

    DEFINE_AR_ALGORITHM(Apriori);

    DEFINE_FD_ALGORITHM(Aid);
    DEFINE_FD_ALGORITHM(Depminer);
    DEFINE_FD_ALGORITHM(DFD);
    DEFINE_FD_ALGORITHM(FastFDs);
    DEFINE_FD_ALGORITHM(FDep);
    DEFINE_FD_ALGORITHM(FdMine);
    DEFINE_FD_ALGORITHM(FUN);
    DEFINE_FD_ALGORITHM(HyFD);
    DEFINE_FD_ALGORITHM(Pyro);
    DEFINE_FD_ALGORITHM(Tane);
}
#undef DEFINE_FD_ALGORITHM
#undef DEFINE_AR_ALGORITHM
#undef DEFINE_ALGORITHM_BASE
#undef DEFINE_ALGORITHM

}  // namespace python_bindings
