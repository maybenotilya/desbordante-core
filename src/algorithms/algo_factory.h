#pragma once

#include <enum.h>

#include <boost/any.hpp>
#include <boost/mp11/algorithm.hpp>

#include "algorithms.h"
#include "options/names.h"
#include "typo_miner.h"

namespace algos {

BETTER_ENUM(AlgoMiningType, char,
#if 1
    fd = 0,
    typos,
    ar,
    metric,
    stats
#else
    fd = 0, /* Functional dependency mining */
    cfd,    /* Conditional functional dependency mining */
    ar,     /* Association rule mining */
    key,    /* Key mining */
    typos,  /* Typo mining */
    metric, /* Metric functional dependency verifying */
    stats   /* Statistic mining */
#endif
);

/* Enumeration of all supported algorithms. If you implemented new algorithm
 * please add new corresponding value to this enum.
 * NOTE: algorithm string name represenation is taken from value in this enum,
 * so name it appropriately (lowercase and without additional symbols).
 */
BETTER_ENUM(Algo, char,
    /* Functional dependency mining algorithms */
    depminer = 0,
    dfd,
    fastfds,
    fdep,
    fdmine,
    pyro,
    tane,
    fun,
    hyfd,
    aidfd,

    /* Association rules mining algorithms */
    apriori,

    /* Metric verifier algorithm */
    metric,

    /* Statistic algorithms */
    stats
);

using StdParamsMap = std::unordered_map<std::string, boost::any>;
using AlgorithmTypesTuple =
        std::tuple<Depminer, DFD, FastFDs, FDep, Fd_mine, Pyro, Tane, FUN, hyfd::HyFD, Aid>;
using ArAlgorithmTuplesType = std::tuple<Apriori>;

namespace details {

namespace onam = algos::config::names;

template <typename AlgorithmBase = Primitive, typename AlgorithmsToSelect = AlgorithmTypesTuple,
          typename EnumType, typename... Args>
auto CreatePrimitiveInstanceImpl(EnumType const enum_value, Args&&... args) {
    auto const create = [&args...](auto I) -> std::unique_ptr<AlgorithmBase> {
        using AlgorithmType = std::tuple_element_t<I, AlgorithmsToSelect>;
        return std::make_unique<AlgorithmType>(std::forward<Args>(args)...);
    };

    return boost::mp11::mp_with_index<std::tuple_size<AlgorithmsToSelect>>((size_t)enum_value,
                                                                           create);
}

template <typename Wrapper, typename AlgorithmBase = Primitive,
          typename AlgorithmsToWrap = AlgorithmTypesTuple, typename EnumType, typename... Args>
auto CreateAlgoWrapperInstanceImpl(EnumType const enum_value, Args&&... args) {
    static_assert(std::is_base_of_v<AlgorithmBase, Wrapper>,
                  "Wrapper should be derived from AlgorithmBase");
    auto const create = [&args...](auto I) -> std::unique_ptr<AlgorithmBase> {
        using AlgorithmType = std::tuple_element_t<I, AlgorithmsToWrap>;
        return Wrapper::template CreateFrom<AlgorithmType>(std::forward<Args>(args)...);
    };

    return boost::mp11::mp_with_index<std::tuple_size<AlgorithmsToWrap>>((size_t)enum_value,
                                                                         create);
}

template <typename T, typename ParamsMap>
T ExtractParamFromMap(ParamsMap& params, std::string const& param_name) {
    auto it = params.find(param_name);
    if (it == params.end()) {
        /* Throw an exception here? Or validate parameters somewhere else (main?)? */
        assert(0);
    }
    if constexpr (std::is_same_v<ParamsMap, StdParamsMap>) {
        return boost::any_cast<T>(params.extract(it).mapped());
    } else {
        return params.extract(it).mapped().template as<T>();
    }
}

/* Really cumbersome, also copying parameter names and types throughout the project
 * (here, main, pyro and tane params). It will be really hard to maintain, need to fix this
 */
template <typename ParamsMap>
FDAlgorithm::Config CreateFDAlgorithmConfigFromMap(ParamsMap params) {
    FDAlgorithm::Config c;

    c.data = std::filesystem::current_path() / "input_data" /
             ExtractParamFromMap<std::string>(params, onam::kData);
    c.separator = ExtractParamFromMap<char>(params, onam::kSeparator);
    c.has_header = ExtractParamFromMap<bool>(params, onam::kHasHeader);
    c.is_null_equal_null = ExtractParamFromMap<bool>(params, onam::kEqualNulls);
    c.max_lhs = ExtractParamFromMap<unsigned int>(params, onam::kMaximumLhs);
    c.parallelism = ExtractParamFromMap<ushort>(params, onam::kThreads);

    /* Is it correct to insert all specified parameters into the algorithm config, and not just the
     * necessary ones? It is definitely simpler, so for now leaving it like this
     */
    for (auto it = params.begin(); it != params.end();) {
        auto node = params.extract(it++);
        if constexpr (std::is_same_v<ParamsMap, StdParamsMap>) {
            c.special_params.emplace(std::move(node.key()), std::move(node.mapped()));
        } else {
            /* ParamsMap == boost::program_options::variable_map */
            c.special_params.emplace(std::move(node.key()), std::move(node.mapped().value()));
        }
    }

    return c;
}

template <typename ParamsMap>
ARAlgorithm::Config CreateArAlgorithmConfigFromMap(ParamsMap params) {
    ARAlgorithm::Config c;

    c.data = std::filesystem::current_path() / "input_data" /
             ExtractParamFromMap<std::string>(params, onam::kData);
    c.separator = ExtractParamFromMap<char>(params, onam::kSeparator);
    c.has_header = ExtractParamFromMap<bool>(params, onam::kHasHeader);
    c.minsup = ExtractParamFromMap<double>(params, onam::kMinimumSupport);
    c.minconf = ExtractParamFromMap<double>(params, onam::kMinimumConfidence);

    std::shared_ptr<model::InputFormat> input_format;
    auto const input_format_arg = ExtractParamFromMap<std::string>(params, onam::kInputFormat);
    if (input_format_arg == "singular") {
        unsigned const tid_column_index = ExtractParamFromMap<unsigned>(params, onam::kTIdColumnIndex);
        unsigned const item_column_index =
            ExtractParamFromMap<unsigned>(params, onam::kItemColumnIndex);
        input_format = std::make_shared<model::Singular>(tid_column_index, item_column_index);
    } else if (input_format_arg == "tabular") {
        bool const first_column_tid = ExtractParamFromMap<bool>(params, onam::kFirstColumnTId);
        input_format = std::make_shared<model::Tabular>(first_column_tid);
    } else {
        throw std::invalid_argument("\"" + input_format_arg + "\"" +
                                    " format is not supported in AR mining");
    }
    c.input_format = std::move(input_format);

    return c;
}

template <typename ParamsMap>
std::unique_ptr<Primitive> CreateFDAlgorithmInstance(Algo const algo, ParamsMap&& params) {
    FDAlgorithm::Config const config =
        CreateFDAlgorithmConfigFromMap(std::forward<ParamsMap>(params));

    return details::CreatePrimitiveInstanceImpl(algo, config);
}

template <typename ParamsMap>
std::unique_ptr<Primitive> CreateTypoMinerInstance(Algo const algo, ParamsMap&& params) {
    /* Typos miner has FDAlgorithm configuration */
    FDAlgorithm::Config const config =
        CreateFDAlgorithmConfigFromMap(std::forward<ParamsMap>(params));

    return details::CreateAlgoWrapperInstanceImpl<TypoMiner>(algo, config);
}

template <typename ParamsMap>
std::unique_ptr<Primitive> CreateArAlgorithmInstance(/*Algo const algo, */ ParamsMap&& params) {
    ARAlgorithm::Config const config =
        CreateArAlgorithmConfigFromMap(std::forward<ParamsMap>(params));

    // return details::CreatePrimitiveInstanceImpl<Primitive, ArAlgorithmTuplesType>(algo, config);

    /* Temporary fix. Template function CreatePrimitiveInstanceImpl does not compile with the new
     * config type ARAlgorithm::Config, even though Apriori algo has been added to Algo enum.
     * So I can suggest two solutions. The first is to implement some base class for the configs
     * (but I'm  not sure if it will work properly) or for Algo enum (I think it is complicated
     * too). The other solution is to add a new ArAlgo enum with an AR algorithms and change
     * CreateAlgorithmInstance function such that it could create enum instance depending on
     * the passed task type.
     */
    return std::make_unique<Apriori>(config);
}

template <typename ParamsMap>
std::unique_ptr<Primitive> CreateCsvStatsInstance(ParamsMap&& params) {
    FDAlgorithm::Config const config =
        CreateFDAlgorithmConfigFromMap(std::forward<ParamsMap>(params));
    return std::make_unique<CsvStats>(config);
}

}  // namespace details

template <typename ParamsMap>
std::unique_ptr<Primitive> CreateAlgorithmInstance(AlgoMiningType const task, Algo const algo,
                                                   ParamsMap&& params) {
    switch (task) {
    case AlgoMiningType::fd:
        return details::CreateFDAlgorithmInstance(algo, std::forward<ParamsMap>(params));
    case AlgoMiningType::typos:
        return details::CreateTypoMinerInstance(algo, std::forward<ParamsMap>(params));
    case AlgoMiningType::ar:
        return details::CreateArAlgorithmInstance(/*algo, */ std::forward<ParamsMap>(params));
    case AlgoMiningType::metric:
        return std::make_unique<MetricVerifier>();
    case AlgoMiningType::stats:
        return details::CreateCsvStatsInstance(std::forward<ParamsMap>(params));
    default:
        throw std::logic_error(task._to_string() + std::string(" task type is not supported yet."));
    }
}

template <typename ParamsMap>
std::unique_ptr<Primitive> CreateAlgorithmInstance(std::string const& task_name,
                                                   std::string const& algo_name,
                                                   ParamsMap&& params) {
    AlgoMiningType const task = AlgoMiningType::_from_string(task_name.c_str());
    Algo const algo = Algo::_from_string(algo_name.c_str());
    return CreateAlgorithmInstance(task, algo, std::forward<ParamsMap>(params));
}

template <typename ParamsMap>
void ConfigureFromMap(Primitive& primitive, ParamsMap const& params) {
    std::unordered_set<std::string> needed;
    while (!(needed = primitive.GetNeededOptions()).empty()) {
        for (std::string const& el : needed) {
            auto it = params.find(el);
            if (it == params.end()) {
                primitive.SetOption(el);
            }
            else {
                if constexpr (std::is_same_v<ParamsMap, StdParamsMap>) {
                    primitive.SetOption(el, it->second);
                } else {  // po map
                    primitive.SetOption(el, params.at(el).value());
                }
            }
        }
    }
}

}  // namespace algos
