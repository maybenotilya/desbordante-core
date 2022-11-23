#pragma once

#include <enum.h>

#include <boost/any.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/program_options.hpp>

#include "algorithms.h"
#include "option_names.h"
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
std::unique_ptr<Primitive> CreateCsvStatsInstance(ParamsMap&& params) {
    FDAlgorithm::Config const config =
        CreateFDAlgorithmConfigFromMap(std::forward<ParamsMap>(params));
    return std::make_unique<CsvStats>(config);
}

template <typename ParamsMap>
boost::any GetParamFromMap(ParamsMap const& params, std::string const& param_name) {
    if constexpr (std::is_same_v<ParamsMap, boost::program_options::variables_map>) {
        return params.at(param_name).value();
    }
    else {
        return params.at(param_name);
    }
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
                primitive.SetOption(el, GetParamFromMap<ParamsMap>(params, el));
            }
        }
    }
}

template <typename PrimitiveType, typename ParamsMap>
std::unique_ptr<PrimitiveType> CreateAndLoadPrimitive(ParamsMap&& params) {
    auto prim = std::make_unique<PrimitiveType>();
    ConfigureFromMap(*prim, params);
    auto parser = CSVParser(
            boost::any_cast<std::string>(GetParamFromMap<ParamsMap>(params, config::names::kData)),
            boost::any_cast<char>(GetParamFromMap<ParamsMap>(params, config::names::kSeparator)),
            boost::any_cast<bool>(GetParamFromMap<ParamsMap>(params, config::names::kHasHeader)));
    prim->Fit(parser);
    ConfigureFromMap(*prim, params);
    return prim;
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
        return details::CreateAndLoadPrimitive<Apriori>(std::forward<ParamsMap>(params)); // temporary
    case AlgoMiningType::metric:
        return details::CreateAndLoadPrimitive<MetricVerifier>(std::forward<ParamsMap>(params));
    case AlgoMiningType::stats:
        return details::CreateAndLoadPrimitive<CsvStats>(std::forward<ParamsMap>(params));
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

}  // namespace algos
