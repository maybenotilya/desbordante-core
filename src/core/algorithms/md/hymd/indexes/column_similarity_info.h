#pragma once

#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/indexes/similarity_index.h"
#include "algorithms/md/hymd/indexes/similarity_matrix.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"

namespace algos::hymd::indexes {
struct ColumnSimilarityInfo {
    std::vector<model::md::DecisionBoundary> natural_decision_boundaries;
    preprocessing::Similarity lowest_similarity;
    indexes::SimilarityMatrix similarity_matrix;
    indexes::SimilarityIndex similarity_index;
};
}  // namespace algos::hymd::indexes
