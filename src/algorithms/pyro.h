#pragma once

#include <list>
#include <mutex>

#include "dependency_consumer.h"
#include "option_type.h"
#include "pli_based_fd_algorithm.h"
#include "search_space.h"

namespace algos {

class Pyro : public DependencyConsumer, public PliBasedFDAlgorithm {
private:
    static config::OptionType<decltype(Configuration::seed)> SeedOpt;

    std::list<std::unique_ptr<SearchSpace>> search_spaces_;

    CachingMethod caching_method_ = CachingMethod::kCoin;
    CacheEvictionMethod eviction_method_ = CacheEvictionMethod::kDefault;
    double caching_method_value_;

    Configuration configuration_;

    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    unsigned long long ExecuteInternal() final;

public:
    Pyro();
};

}  // namespace algos
