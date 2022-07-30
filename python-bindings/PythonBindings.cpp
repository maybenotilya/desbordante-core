#include <boost/python/object.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/str.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/module.hpp>

#include "Primitive.h"

namespace python_bindings {

namespace py = boost::python;

class PyPrimitive {
public:
    void fit(py::object obj)
    {

    }

    void fit(const py::str & path, const py::str & delimiter, const py::object & has_header)
    {
        std::string delim = py::extract<std::string>(delimiter)();
        if (delim.length() != 1)
            throw std::runtime_error("delimiter length must be 1");
        path_ = py::extract<std::string>(path)();
        delimiter_ = delim[0];
        has_header_ = py::extract<bool>(has_header)();
    }

    unsigned long long execute(boost::python::tuple args, boost::python::dict kwargs)
    {
        return 0;
    }

private:
    std::shared_ptr<algos::Primitive> prim;
    std::string path_;
    char delimiter_;
    bool has_header_;
};

BOOST_PYTHON_MODULE(desbordante)
{

}

}  // namespace python_bindings
