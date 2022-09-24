#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/init.hpp>
#include <boost/python/module.hpp>
#include <boost/python/object.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>
#include <easylogging++.h>

#include "AlgoFactory.h"
#include "DictToParamsMap.h"
#include "Primitive.h"
#include "ProgramOptionStrings.h"

INITIALIZE_EASYLOGGINGPP

namespace python_bindings {

namespace posr = program_option_strings;
namespace py = boost::python;

class PyPrimitive {
public:
    /*void fit(py::object obj)
    {

    }*/

    void fit(const py::str & path, const py::str & delimiter, const py::object & has_header)
    {
    }

    static py::object execute(const py::tuple & _, const py::dict & kwargs)
    {
        auto params_map = GetParamsMap(kwargs);
        std::string algorithm;
        if (params_map.find(posr::kAlgorithm) != params_map.end())
            algorithm = boost::any_cast<std::string>(params_map[posr::kAlgorithm]);
        else
            algorithm = "metric";
        auto prim = algos::CreateAlgorithmInstance(
            boost::any_cast<std::string>(params_map[posr::kTask]),
            algorithm,
            params_map);
        return py::object(prim->Execute());
    }

private:
    std::shared_ptr<algos::Primitive> prim;
};

BOOST_PYTHON_MODULE(desbordante)
{
    py::class_<PyPrimitive>("Primitive", py::init())
        .def("fit", &PyPrimitive::fit)
        .def("execute", py::raw_function(&PyPrimitive::execute, 1));
}

}  // namespace python_bindings
