#pragma once

#include <string>

#include "option_names.h"
#include "pli_based_fd_algorithm.h"
#include "position_list_index.h"
#include "relation_data.h"

namespace algos {

class Tane : public PliBasedFDAlgorithm {
private:
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    unsigned long long ExecuteInternal() final;

public:
    //TODO: these consts should go in class (or struct) Configuration
    double max_fd_error_ = 0.01;
    double max_ucc_error_ = 0.01;
    unsigned int max_lhs_ = -1;

    int count_of_fd_ = 0;
    int count_of_ucc_ = 0;
    long apriori_millis_ = 0;

    Tane();

    static double CalculateZeroAryFdError(ColumnData const* rhs,
                                          ColumnLayoutRelationData const* relation_data);
    static double CalculateFdError(util::PositionListIndex const* lhs_pli,
                                   util::PositionListIndex const* joint_pli,
                                   ColumnLayoutRelationData const* relation_data);
    static double CalculateUccError(util::PositionListIndex const* pli,
                                    ColumnLayoutRelationData const* relation_data);

    //static double round(double error) { return ((int)(error * 32768) + 1)/ 32768.0; }

    void RegisterFd(Vertical const& lhs, Column const* rhs,
                    double error, RelationalSchema const* schema);
    // void RegisterFd(Vertical const* lhs, Column const* rhs, double error, RelationalSchema const* schema);
    void RegisterUcc(Vertical const& key, double error, RelationalSchema const* schema);
};

}  // namespace algos
