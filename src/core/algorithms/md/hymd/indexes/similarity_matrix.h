#pragma once

#include <unordered_map>

#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/table_identifiers.h"

namespace algos::hymd::indexes {
using SimilarityMatrixRow = std::unordered_map<ValueIdentifier, preprocessing::Similarity>;
using SimilarityMatrix = std::unordered_map<ValueIdentifier, SimilarityMatrixRow>;
}  // namespace algos::hymd::indexes
