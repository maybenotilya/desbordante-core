#include <easylogging++.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "algo_factory.h"
#include "enum_to_available_values.h"
#include "metric_verifier_enums.h"
#include "options/descriptions.h"
#include "options/names.h"

namespace po = boost::program_options;
namespace onam = algos::config::names;
namespace descriptions = algos::config::descriptions;

INITIALIZE_EASYLOGGINGPP

namespace algos {

void validate(boost::any& v, const std::vector<std::string>& values, Metric*, int) {
    const std::string& s = po::validators::get_single_string(values);
    try {
        v = boost::any(Metric::_from_string(s.c_str()));
    } catch (std::runtime_error&e) {
        throw po::validation_error(po::validation_error::invalid_option_value);
    }
}

void validate(boost::any& v, const std::vector<std::string>& values, MetricAlgo*, int) {
    const std::string& s = po::validators::get_single_string(values);
    try {
        v = boost::any(MetricAlgo::_from_string(s.c_str()));
    } catch (std::runtime_error &e) {
        throw po::validation_error(po::validation_error::invalid_option_value);
    }
}

}  // namespace algos

static bool CheckOptions(std::string const& task, std::string const& alg, double error) {
    if (!algos::AlgoMiningType::_is_valid(task.c_str())) {
        std::cout << "ERROR: no matching task."
                     " Available tasks (primitives to mine) are:\n" +
                     EnumToAvailableValues<algos::AlgoMiningType>() + '\n';
        return false;
    }

    if (!algos::Algo::_is_valid(alg.c_str())) {
        std::cout << "ERROR: no matching algorithm."
                     " Available algorithms are:\n" +
                     EnumToAvailableValues<algos::Algo>() + '\n';
        return false;
    }

    if (error > 1 || error < 0) {
        std::cout << "ERROR: error should be between 0 and 1.\n";
    }

    return true;
}

int main(int argc, char const* argv[]) {
    std::string algo;
    std::string dataset;
    std::string task;
    char separator = ',';
    bool has_header = true;
    int seed = 0;
    double error = 0.0;
    unsigned int max_lhs = -1;
    ushort threads = 0;
    bool is_null_equal_null = true;

    /*Options for association rule mining algorithms*/
    double minsup = 0.0;
    double minconf = 0.0;
    std::string ar_input_format;
    unsigned tid_column_index = 0;
    unsigned item_column_index = 1;
    bool has_transaction_id = false;

    /*Options for metric verifier algorithm*/
    algos::Metric metric = algos::Metric::_values()[0];
    algos::MetricAlgo metric_algo = algos::MetricAlgo::_values()[0];
    std::vector<unsigned int> lhs_indices;
    std::vector<unsigned int> rhs_indices;
    long double parameter = 0;
    unsigned int q = 2;
    bool dist_to_null_infinity = false;

    std::string const algo_desc = "algorithm to use\n" +
                                  EnumToAvailableValues<algos::Algo>() +
                                  " for FD mining.";
    std::string const task_desc = "type of dependency to mine\n" +
                                  EnumToAvailableValues<algos::AlgoMiningType>();
    constexpr auto help_opt = "help";
    constexpr auto algorithm_opt = "algorithm";
    constexpr auto task_opt = "task";
    std::string const separator_opt = std::string(onam::kSeparator) + ",s";

    po::options_description info_options("Desbordante information options");
    info_options.add_options()
        (help_opt, "print the help message and exit")
        // --version, if needed, goes here too
        ;

    po::options_description general_options("General options");
    general_options.add_options()
        (task_opt, po::value<std::string>(&task), task_desc.c_str())
        (
            algorithm_opt, po::value<std::string>(&algo), algo_desc.c_str())
        (onam::kData, po::value<std::string>(&dataset), descriptions::kDData)
        (
            separator_opt.c_str(), po::value<char>(&separator)->default_value(separator),
            descriptions::kDSeparator)
        (onam::kHasHeader, po::value<bool>(&has_header)->default_value(has_header),
            descriptions::kDHasHeader)
        (onam::kEqualNulls, po::value<bool>(&is_null_equal_null)->default_value(true),
            descriptions::kDEqualNulls)
        (onam::kThreads, po::value<ushort>(&threads)->default_value(threads),
            descriptions::kDThreads)
        ;

    po::options_description typos_fd_options("Typo mining/FD options");
    typos_fd_options.add_options()
        (onam::kError, po::value<double>(&error)->default_value(error), descriptions::kDError)
        (onam::kMaximumLhs, po::value<unsigned int>(&max_lhs)->default_value(max_lhs),
            descriptions::kDMaximumLhs)
        (onam::kSeed, po::value<int>(&seed)->default_value(seed), descriptions::kDSeed)
        ;

    po::options_description ar_options("AR options");
    ar_options.add_options()
        (onam::kMinimumSupport, po::value<double>(&minsup), descriptions::kDMinimumSupport)
        (onam::kMinimumConfidence, po::value<double>(&minconf), descriptions::kDMinimumConfidence)
        (onam::kInputFormat, po::value<string>(&ar_input_format), descriptions::kDInputFormat)
        ;

    po::options_description ar_singular_options("AR \"singular\" input format options");
    ar_singular_options.add_options()
        (onam::kTIdColumnIndex, po::value<unsigned>(&tid_column_index)->default_value(0),
            descriptions::kDTIdColumnIndex)
        (onam::kItemColumnIndex, po::value<unsigned>(&item_column_index)->default_value(1),
            descriptions::kDItemColumnIndex)
        ;

    po::options_description ar_tabular_options("AR \"tabular\" input format options");
    ar_tabular_options.add_options()
        (onam::kFirstColumnTId, po::bool_switch(&has_transaction_id),
             descriptions::kDFirstColumnTId)
        ;

    ar_options.add(ar_singular_options).add(ar_tabular_options);

    po::options_description mfd_options("MFD options");
    mfd_options.add_options()
        (onam::kMetric, po::value<algos::Metric>(&metric), descriptions::kDMetric)
        (onam::kMetricAlgorithm, po::value<algos::MetricAlgo>(&metric_algo),
            descriptions::kDMetricAlgorithm)
        (onam::kLhsIndices, po::value<std::vector<unsigned int>>(&lhs_indices)->multitoken(),
            descriptions::kDLhsIndices)
        (onam::kRhsIndices, po::value<std::vector<unsigned int>>(&rhs_indices)->multitoken(),
            descriptions::kDRhsIndices)
        (onam::kParameter, po::value<long double>(&parameter), descriptions::kDParameter)
        (onam::kDistToNullIsInfinity, po::bool_switch(&dist_to_null_infinity),
            descriptions::kDDistToNullIsInfinity)
        ;

    po::options_description cosine_options("Cosine metric options");
    cosine_options.add_options()
        (onam::kQGramLength, po::value<unsigned int>(&q)->default_value(2),
            descriptions::kDQGramLength)
        ;

    mfd_options.add(cosine_options);

    po::options_description all_options("Allowed options");
    all_options.add(info_options).add(general_options).add(typos_fd_options)
        .add(mfd_options).add(ar_options);

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, all_options), vm);
        po::notify(vm);
    } catch (po::error &e) {
        std::cout << e.what() << std::endl;
        return 0;
    }

    if (vm.count(help_opt))
    {
        std::cout << all_options << std::endl;
        return 0;
    }

    el::Loggers::configureFromGlobal("logging.conf");

    std::transform(algo.begin(), algo.end(), algo.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (!CheckOptions(task, algo, error)) {
        std::cout << all_options << std::endl;
        return 1;
    }

    if (task == "fd" || task == "typos") {
        std::cout << "Input: algorithm \"" << algo
                  << "\" with seed " << std::to_string(seed)
                  << ", error \"" << std::to_string(error)
                  << ", max_lhs \"" << std::to_string(max_lhs)
                  << "\" and dataset \"" << dataset
                  << "\" with separator \'" << separator
                  << "\'. Header is " << (has_header ? "" : "not ") << "present. " << std::endl;
    } else if (task == "ar") {
        std::cout << "Input: algorithm \"" << algo
                  << "\" with min. support threshold \"" << std::to_string(minsup)
                  << "\", min. confidence threshold \"" << std::to_string(minconf)
                  << "\" and dataset \"" << dataset
                  << "\". Input type is \"" << ar_input_format
                  << "\" with separator \'" << separator
                  << "\'. Header is " << (has_header ? "" : "not ") << "present. " << std::endl;
    } else if (task == "metric") {
        algo = "metric";
        auto get_str = [](std::vector<unsigned> const& vec) {
            std::stringstream stream;
            std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(stream, " "));
            string lhs_indices_str = stream.str();
            boost::trim_right(lhs_indices_str);
            return lhs_indices_str;
        };
        std::cout << "Input metric \"" << metric;
        if (metric == +algos::Metric::cosine) std::cout << "\" with q \"" << q;
        std::cout << "\" with parameter \"" << parameter
                  << "\" with LHS indices \"" << get_str(lhs_indices)
                  << "\" with RHS indices \"" << get_str(rhs_indices)
                  << "\" and dataset \"" << dataset
                  << "\" with separator \'" << separator
                  << "\'. Header is " << (has_header ? "" : "not ") << "present. " << std::endl;
    }

    std::unique_ptr<algos::Primitive> algorithm_instance =
        algos::CreateAlgorithmInstance(task, algo, vm);

    try {
        algos::ConfigureFromMap(*algorithm_instance, vm);
        auto parser = CSVParser(dataset, separator, has_header);
        algorithm_instance->Fit(parser);
        algos::ConfigureFromMap(*algorithm_instance, vm);
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
