#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"

namespace algos::hymd::preprocessing::similarity_measure {

class LevenshteinMeasure final : public ImmediateSimilarityMeasure {
    Similarity CalculateSimilarity(std::byte const* value_left,
                                   std::byte const* value_right) const final;

public:
    LevenshteinMeasure(model::md::DecisionBoundary min_sim);
};

}  // namespace algos::hymd::preprocessing::similarity_measure
