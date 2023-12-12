#pragma once

#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "model/types/double_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class ImmediateSimilarityMeasure : public SimilarityMeasure {
    [[nodiscard]] indexes::ColumnSimilarityInfo MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const* clusters_right,
            bool is_null_equal_null) const final;

public:
    ImmediateSimilarityMeasure(std::string name, std::unique_ptr<model::Type> arg_type,
                               model::md::DecisionBoundary min_sim)
        : SimilarityMeasure(std::move(name), std::move(arg_type),
                            std::make_unique<model::DoubleType>(), min_sim){};

    virtual Similarity CalculateSimilarity(std::byte const* value_left,
                                           std::byte const* value_right) const = 0;
};

}  // namespace algos::hymd::preprocessing::similarity_measure
