#pragma once

#include <type_traits>
#include <enum.h>

#include <boost/any.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/program_options.hpp>

#include "algorithms.h"
#include "option_names.h"
#include "primitive_enum.h"
#include "typo_miner.h"

namespace algos {

using StdParamsMap = std::unordered_map<std::string, boost::any>;

namespace details {

template <typename ParamsMap>
boost::any ExtractAnyFromMap(ParamsMap& options, std::string const& option_name) {
    auto it = options.find(option_name);
    if (it == options.end()) {
        throw std::invalid_argument("No option named \"" + option_name
                                    + "\" in parameters.");
    }
    if constexpr (std::is_same_v<typename std::decay<ParamsMap>::type,
                                 boost::program_options::variables_map>) {
        return options.extract(it).mapped().value();
    }
    else {
        return options.extract(it).mapped();
    }
}

template <typename T, typename OptionMap>
T ExtractOptionValue(OptionMap& options, std::string const& option_name) {
    return boost::any_cast<T>(ExtractAnyFromMap(options, option_name));
}

template <typename ParamsMap>
void ConfigureFromMap(Primitive& primitive, ParamsMap& options) {
    std::unordered_set<std::string_view> needed;
    while (!(needed = primitive.GetNeededOptions()).empty()) {
        for (std::string_view const& option_name : needed) {
            try {
                primitive.SetOption(option_name, ExtractAnyFromMap(options,
                                                                   std::string{option_name}));
            } catch (std::invalid_argument&) {
                primitive.SetOption(option_name);
            }
        }
    }
}

template <typename OptionMap>
void LoadPrimitive(Primitive& prim, OptionMap& options) {
    details::ConfigureFromMap(prim, options);
    auto parser = CSVParser{
            ExtractOptionValue<std::string>(options, config::names::kData),
            ExtractOptionValue<char>(options, config::names::kSeparator),
            ExtractOptionValue<bool>(options, config::names::kHasHeader)};
    prim.Fit(parser);
    ConfigureFromMap(prim, options);
}

}  // namespace details

template <typename PrimitiveType, typename OptionMap>
std::unique_ptr<PrimitiveType> CreateAndLoadPrimitive(OptionMap&& options) {
    auto prim = std::make_unique<PrimitiveType>();
    details::LoadPrimitive(*prim, std::forward<OptionMap>(options));
    return prim;
}

template <typename OptionMap>
std::unique_ptr<Primitive> CreatePrimitive(Primitives primitive_enum, OptionMap&& options) {
    std::unique_ptr<Primitive> primitive = CreatePrimitiveInstance(primitive_enum);
    details::LoadPrimitive(*primitive, std::forward<OptionMap>(options));
    return primitive;
}

template <typename OptionMap>
std::unique_ptr<Primitive> CreatePrimitive(std::string const& primitive_name,
                                           OptionMap&& options) {
    Primitives const primitive_enum = Primitives::_from_string(primitive_name.c_str());
    return CreatePrimitive(primitive_enum, std::forward<OptionMap>(options));
}

}  // namespace algos
