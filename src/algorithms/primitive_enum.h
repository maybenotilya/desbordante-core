#pragma once

#include "algorithms.h"

#include <enum.h>

namespace algos {

using PrimitiveTypes = std::tuple<Depminer, DFD, FastFDs, FDep, Fd_mine, Pyro, Tane, FUN,
                                  hyfd::HyFD, Aid, Apriori, MetricVerifier, CsvStats>;

/* Enumeration of all supported algorithms. If you implemented new algorithm
 * please add new corresponding value to this enum.
 * NOTE: algorithm string name represenation is taken from value in this enum,
 * so name it appropriately (lowercase and without additional symbols).
 */
BETTER_ENUM(Primitives, char,
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

template <typename PrimitiveBase = Primitive>
std::unique_ptr<PrimitiveBase> CreatePrimitiveInstance(Primitives primitive) {
    auto const create = [](auto I) -> std::unique_ptr<PrimitiveBase> {
        using PrimitiveType = std::tuple_element_t<I, PrimitiveTypes>;
        if constexpr (std::is_convertible_v<PrimitiveType *, PrimitiveBase *>) {
            return std::make_unique<PrimitiveType>();
        }
        else {
            throw std::invalid_argument("Cannot use "
                                        + boost::typeindex::type_id<PrimitiveType>().pretty_name()
                                        + " as "
                                        + boost::typeindex::type_id<PrimitiveBase>().pretty_name());
        }
    };

    return boost::mp11::mp_with_index<std::tuple_size<PrimitiveTypes>>(
            static_cast<size_t>(primitive), create);
}

template <typename PrimitiveBase>
std::vector<Primitives> GetAllDerived() {
    auto const is_derived = [](auto I) -> bool {
        using PrimitiveType = std::tuple_element_t<I, PrimitiveTypes>;
        return std::is_base_of_v<PrimitiveBase, PrimitiveType>;
    };
    std::vector<Primitives> derived_from_base{};
    for (Primitives prim : Primitives::_values()) {
        if (boost::mp11::mp_with_index<std::tuple_size<PrimitiveTypes>>(static_cast<size_t>(prim),
                                                                        is_derived)) {
            derived_from_base.push_back(prim);
        }
    }
    return derived_from_base;
}

}  // namespace algos
