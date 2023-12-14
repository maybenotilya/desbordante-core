#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"

namespace algos::hymd::preprocessing::similarity_measure {

class LevenshteinSimilarityMeasure final : public SimilarityMeasure {
    bool const is_null_equal_null_;
    model::md::DecisionBoundary const min_sim_;

public:
    [[nodiscard]] indexes::ColumnSimilarityInfo MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right) const final;

    LevenshteinSimilarityMeasure(model::md::DecisionBoundary min_sim, bool is_null_equal_null);
};

}  // namespace algos::hymd::preprocessing::similarity_measure
