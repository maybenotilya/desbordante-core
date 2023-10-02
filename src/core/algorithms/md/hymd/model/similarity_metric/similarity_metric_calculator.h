#pragma once

#include <tuple>

#include "algorithms/md/hymd/types.h"
#include "model/types/type.h"

namespace algos::hymd::model {

class SimilarityMetricCalculator {
protected:
    DecisionBoundsVector decision_bounds_;
    SimilarityMatrix similarity_matrix_;
    SimilarityIndex similarity_index_;

public:
    virtual void MakeIndexes() = 0;
    // Make buf, pass buf to metric, pass proper type to fun in metric during
    // MakeIndexes, set sim metric return type, optionally normalize on left
    // value or all sims, optionally check if not normalized.
    // buf<T> left, buf<T> right
    // std::function<RetType(T const&, T const&)>
    DecisionBoundsVector TakeDecisionBounds() {
        return std::move(decision_bounds_);
    }
    SimilarityMatrix TakeSimilarityMatrix() {
        return std::move(similarity_matrix_);
    }
    SimilarityIndex TakeSimilarityIndex() {
        return std::move(similarity_index_);
    }
};

}  // namespace algos::hymd::model