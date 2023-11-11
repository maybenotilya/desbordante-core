#pragma once

#include <map>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/table_identifiers.h"

namespace algos::hymd::indexes {
using SimInfo = std::map<preprocessing::Similarity, std::vector<RecordIdentifier>>;
using SimilarityIndex = std::unordered_map<ValueIdentifier, SimInfo>;
}  // namespace algos::hymd::indexes
