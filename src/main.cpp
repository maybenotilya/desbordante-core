#include <easylogging++.h>

#include <algorithm>
#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "algo_factory.h"
#include "enum_to_available_values.h"
#include "metric_verifier_enums.h"
#include "option_descriptions.h"
#include "option_names.h"

namespace po = boost::program_options;
namespace onam = algos::config::names;
namespace descriptions = algos::config::descriptions;

INITIALIZE_EASYLOGGINGPP

constexpr auto primitive_opt = "primitive";

namespace algos {

void validate(boost::any& v, const std::vector<std::string>& values, Metric*, int) {
    const std::string& s = po::validators::get_single_string(values);
    try {
        v = boost::any(Metric::_from_string(s.c_str()));
    } catch (std::runtime_error&e) {
        throw po::validation_error(po::validation_error::invalid_option_value,
                                   algos::config::names::kMetric, s);
    }
}

void validate(boost::any& v, const std::vector<std::string>& values, MetricAlgo*, int) {
    const std::string& s = po::validators::get_single_string(values);
    try {
        v = boost::any(MetricAlgo::_from_string(s.c_str()));
    } catch (std::runtime_error &e) {
        throw po::validation_error(po::validation_error::invalid_option_value,
                                   algos::config::names::kMetricAlgorithm, s);
    }
}

void validate(boost::any& v, const std::vector<std::string>& values, Primitives*, int) {
    const std::string& s = po::validators::get_single_string(values);
    try {
        v = boost::any(Primitives::_from_string(s.c_str()));
    } catch (std::runtime_error &e) {
        throw po::validation_error(po::validation_error::invalid_option_value, primitive_opt, s);
    }
}

}  // namespace algos

static bool CheckOptions(std::string const& prim) {
    if (!algos::Primitives::_is_valid(prim.c_str())) {
        std::cout << "ERROR: no matching algorithm."
                     " Available algorithms are:\n" +
                     EnumToAvailableValues<algos::Primitives>() + '\n';
        return false;
    }

    return true;
}

int main(int argc, char const* argv[]) {
    algos::Primitives primitive = algos::Primitives::_values()[0];

    std::string const algo_desc = "algorithm to use for data profiling\n" +
                                  EnumToAvailableValues<algos::Primitives>();
    constexpr auto help_opt = "help";
    std::string const separator_opt = std::string(onam::kSeparator) + ",s";

    po::options_description info_options("Desbordante information options");
    info_options.add_options()
        (help_opt, "print the help message and exit")
        // --version, if needed, goes here too
        ;

    po::options_description general_options("General options");
    general_options.add_options()
        (primitive_opt, po::value<algos::Primitives>(&primitive)->required(), algo_desc.c_str())
        (onam::kData, po::value<std::string>()->required(), descriptions::kDData)
        (separator_opt.c_str(), po::value<char>()->default_value(','), descriptions::kDSeparator)
        (onam::kHasHeader, po::value<bool>()->default_value(true), descriptions::kDHasHeader)
        (onam::kEqualNulls, po::value<bool>(), descriptions::kDEqualNulls)
        (onam::kThreads, po::value<ushort>(), descriptions::kDThreads)
        ;

    po::options_description typos_fd_options("Typo mining/FD options");
    typos_fd_options.add_options()
        (onam::kError, po::value<double>(), descriptions::kDError)
        (onam::kMaximumLhs, po::value<unsigned int>(), descriptions::kDMaximumLhs)
        (onam::kSeed, po::value<int>(), descriptions::kDSeed)
        ;

    po::options_description ar_options("AR options");
    ar_options.add_options()
        (onam::kMinimumSupport, po::value<double>(), descriptions::kDMinimumSupport)
        (onam::kMinimumConfidence, po::value<double>(), descriptions::kDMinimumConfidence)
        (onam::kInputFormat, po::value<string>(), descriptions::kDInputFormat)
        ;

    po::options_description ar_singular_options("AR \"singular\" input format options");
    ar_singular_options.add_options()
        (onam::kTIdColumnIndex, po::value<unsigned>(), descriptions::kDTIdColumnIndex)
        (onam::kItemColumnIndex, po::value<unsigned>(), descriptions::kDItemColumnIndex)
        ;

    po::options_description ar_tabular_options("AR \"tabular\" input format options");
    ar_tabular_options.add_options()
        (onam::kFirstColumnTId, po::bool_switch(), descriptions::kDFirstColumnTId)
        ;

    ar_options.add(ar_singular_options).add(ar_tabular_options);

    po::options_description mfd_options("MFD options");
    mfd_options.add_options()
        (onam::kMetric, po::value<algos::Metric>(), descriptions::kDMetric)
        (onam::kMetricAlgorithm, po::value<algos::MetricAlgo>(), descriptions::kDMetricAlgorithm)
        (onam::kLhsIndices, po::value<std::vector<unsigned int>>()->multitoken(),
            descriptions::kDLhsIndices)
        (onam::kRhsIndices, po::value<std::vector<unsigned int>>()->multitoken(),
            descriptions::kDRhsIndices)
        (onam::kParameter, po::value<long double>(), descriptions::kDParameter)
        (onam::kDistToNullIsInfinity, po::bool_switch(), descriptions::kDDistToNullIsInfinity)
        ;

    po::options_description cosine_options("Cosine metric options");
    cosine_options.add_options()
        (onam::kQGramLength, po::value<unsigned int>(), descriptions::kDQGramLength)
        ;

    mfd_options.add(cosine_options);

    po::options_description all_options("Allowed options");
    all_options.add(info_options).add(general_options).add(typos_fd_options)
        .add(mfd_options).add(ar_options);

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, all_options), vm);
    } catch (po::error &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    if (vm.count(help_opt))
    {
        std::cout << all_options << std::endl;
        return 0;
    }
    try {
        po::notify(vm);
    } catch (po::error &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    el::Loggers::configureFromGlobal("logging.conf");

    std::unique_ptr<algos::Primitive> algorithm_instance;
    try {
        algorithm_instance = algos::CreatePrimitive(primitive, vm);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    try {
        unsigned long long elapsed_time = algorithm_instance->Execute();
        std::cout << "> ELAPSED TIME: " << elapsed_time << std::endl;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
