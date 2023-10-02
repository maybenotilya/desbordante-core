#pragma once

#include "algorithms/md/hymd/model/similarity_metric/immediate_metric_calculator.h"
#include "algorithms/md/hymd/model/similarity_metric/similarity_metric.h"

namespace algos::hymd::model {

template <typename T>
class ImmediateSimilarityMetric : SimilarityMetric<T, double> {
    [[nodiscard]] std::shared_ptr<SimilarityMetricCalculator> MakeCalculatorInternal(
            std::shared_ptr<ValueInfo<T> const> value_info_left,
            std::shared_ptr<ValueInfo<T> const> value_info_right,
            std::vector<PliCluster> const* clusters_right, double min_sim,
            bool is_null_equal_null) const {
        using SimilarityMetric<T, double>::get_similarity_;
        return std::make_shared<ImmediateMetricCalculator<T>>(
                std::move(value_info_left), std::move(value_info_right), get_similarity_,
                clusters_right, min_sim, is_null_equal_null);
    }

public:
    ImmediateSimilarityMetric(std::string name,
                              std::function<double(T const&, T const&)> get_similarity)
        : SimilarityMetric<T, double>(std::move(name), std::move(get_similarity)) {}
};

}
