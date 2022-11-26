#pragma once

#include <random>
#include <stack>

#include "common_options.h"
#include "pli_based_fd_algorithm.h"
#include "vertical.h"
#include "dfd/partition_storage/partition_storage.h"

namespace algos {

class DFD : public PliBasedFDAlgorithm {
private:
    std::unique_ptr<PartitionStorage> partition_storage_;
    std::vector<Vertical> unique_columns_;

    config::ThreadNumType number_of_threads_;

    void MakeExecuteOptsAvailable() final;
    void RegisterOptions();
    unsigned long long ExecuteInternal() final;

public:
    DFD();
};

}  // namespace algos
