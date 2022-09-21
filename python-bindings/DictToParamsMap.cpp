#include <unordered_map>
#include <string>
#include <boost/python/dict.hpp>
#include <boost/any.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/extract.hpp>

#include "ProgramOptionStrings.h"

namespace python_bindings
{
namespace py = boost::python;
namespace posr = program_option_strings;
using ParamsMap = std::unordered_map<std::string, boost::any>;

template <typename Expected>
boost::any ConvertPyToAny(const py::object & obj)
{
    return boost::any(py::extract<Expected>(obj)());
}

template <>
/*
——No partial function specialization?——
⠀⣞⢽⢪⢣⢣⢣⢫⡺⡵⣝⡮⣗⢷⢽⢽⢽⣮⡷⡽⣜⣜⢮⢺⣜⢷⢽⢝⡽⣝
⠸⡸⠜⠕⠕⠁⢁⢇⢏⢽⢺⣪⡳⡝⣎⣏⢯⢞⡿⣟⣷⣳⢯⡷⣽⢽⢯⣳⣫⠇
⠀⠀⢀⢀⢄⢬⢪⡪⡎⣆⡈⠚⠜⠕⠇⠗⠝⢕⢯⢫⣞⣯⣿⣻⡽⣏⢗⣗⠏⠀
⠀⠪⡪⡪⣪⢪⢺⢸⢢⢓⢆⢤⢀⠀⠀⠀⠀⠈⢊⢞⡾⣿⡯⣏⢮⠷⠁⠀⠀
⠀⠀⠀⠈⠊⠆⡃⠕⢕⢇⢇⢇⢇⢇⢏⢎⢎⢆⢄⠀⢑⣽⣿⢝⠲⠉⠀⠀⠀⠀
⠀⠀⠀⠀⠀⡿⠂⠠⠀⡇⢇⠕⢈⣀⠀⠁⠡⠣⡣⡫⣂⣿⠯⢪⠰⠂⠀⠀⠀⠀
⠀⠀⠀⠀⡦⡙⡂⢀⢤⢣⠣⡈⣾⡃⠠⠄⠀⡄⢱⣌⣶⢏⢊⠂⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⢝⡲⣜⡮⡏⢎⢌⢂⠙⠢⠐⢀⢘⢵⣽⣿⡿⠁⠁⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠨⣺⡺⡕⡕⡱⡑⡆⡕⡅⡕⡜⡼⢽⡻⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⣼⣳⣫⣾⣵⣗⡵⡱⡡⢣⢑⢕⢜⢕⡝⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⣴⣿⣾⣿⣿⣿⡿⡽⡑⢌⠪⡢⡣⣣⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⡟⡾⣿⢿⢿⢵⣽⣾⣼⣘⢸⢸⣞⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠁⠇⠡⠩⡫⢿⣝⡻⡮⣒⢽⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
—————————————————————————————
*/
boost::any ConvertPyToAny<std::vector<unsigned int>>(const py::object & obj)
{
    py::list lst = py::extract<py::list>(obj)();
    std::vector<unsigned int> vec{};
    auto len = boost::python::len(lst);
    for (boost::python::ssize_t i = 0; i < len; ++i) {
        vec.push_back(py::extract<unsigned int>(lst[i])());
    }
    return vec;
}

std::unordered_map<std::string, std::function<boost::any(py::object)>> conv_map = {
    {posr::kTask, ConvertPyToAny<std::string>},
    {posr::kAlgorithm, ConvertPyToAny<std::string>},
    {posr::kData, ConvertPyToAny<std::string>},
    {posr::kSeparatorConfig, ConvertPyToAny<char>},
    {posr::kHasHeader, ConvertPyToAny<bool>},
    {posr::kEqualNulls, ConvertPyToAny<bool>},
    {posr::kThreads, ConvertPyToAny<ushort>},
    {posr::kError, ConvertPyToAny<double>},
    {posr::kMaximumLhs, ConvertPyToAny<unsigned int>},
    {posr::kSeed, ConvertPyToAny<int>},
    {posr::kMinimumSupport, ConvertPyToAny<double>},
    {posr::kMinimumConfidence, ConvertPyToAny<double>},
    {posr::kInputFormat, ConvertPyToAny<std::string>},
    {posr::kTIdColumnIndex, ConvertPyToAny<unsigned int>},
    {posr::kItemColumnIndex, ConvertPyToAny<unsigned int>},
    {posr::kFirstColumnTId, ConvertPyToAny<bool>},
    {posr::kMetric, ConvertPyToAny<std::string>},
    {posr::kLhsIndices, ConvertPyToAny<std::vector<unsigned int>>},
    {posr::kRhsIndices, ConvertPyToAny<std::vector<unsigned int>>},
    {posr::kParameter, ConvertPyToAny<long double>},
    {posr::kDistToNullIsInfinity, ConvertPyToAny<bool>},
    {posr::kQGramLength, ConvertPyToAny<unsigned int>},
    {posr::kMetricAlgorithm, ConvertPyToAny<std::string>},
    {posr::kRadius, ConvertPyToAny<double>},
    {posr::kRatio, ConvertPyToAny<double>},
};

ParamsMap GetParamsMap(const py::dict & dict)
{
    auto params_map = ParamsMap();
    auto items = dict.attr("items")();
    for (auto it = py::stl_input_iterator<py::tuple>(items);
         it != py::stl_input_iterator<py::tuple>();
         ++it)
    {
        py::tuple kv = *it;
        auto key = kv[0];
        auto value = kv[1];
        auto key_str = py::extract<std::string>(key);
        boost::any val_any = conv_map[key_str](value);
        params_map.emplace(key_str, val_any);
    }

    return params_map;
}

}  // namespace python_bindings
