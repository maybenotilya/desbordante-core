#pragma once

#include "algorithms/md/hymd/model/similarity_measure/similarity_measure.h"

namespace algos::hymd::model {

class ImmediateSimilarityMeasure final : public SimilarityMeasure {
    bool const should_check_ = false;

    [[nodiscard]] std::tuple<DecisionBoundsVector, SimilarityMatrix, SimilarityIndex> MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<PliCluster> const* clusters_right, double min_sim,
            bool is_null_equal_null) const final;

public:
    ImmediateSimilarityMeasure(std::string name, std::unique_ptr<::model::Type> arg_type,
                              std::unique_ptr<::model::INumericType> ret_type,
                              SimilarityFunction compute_similarity, bool should_check)
        : SimilarityMeasure(std::move(name), std::move(arg_type), std::move(ret_type),
                           std::move(compute_similarity)),
          should_check_(should_check){};
};

}
