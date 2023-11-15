#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/types/double_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class ImmediateSimilarityMeasure final : public SimilarityMeasure {
    bool const should_check_ = false;

    [[nodiscard]] indexes::ColumnSimilarityInfo MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const* clusters_right,
            model::md::DecisionBoundary min_sim, bool is_null_equal_null) const final;

public:
    ImmediateSimilarityMeasure(std::string name, std::unique_ptr<model::Type> arg_type,
                               SimilarityFunction compute_similarity, bool should_check)
        : SimilarityMeasure(std::move(name), std::move(arg_type),
                            std::make_unique<model::DoubleType>(), std::move(compute_similarity)),
          should_check_(should_check){};
};

}  // namespace algos::hymd::preprocessing::similarity_measure
