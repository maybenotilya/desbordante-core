#pragma once

#include <cstddef>
#include <map>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd {
using ValueIdentifier = size_t;
using RecordIdentifier = size_t;
using PliCluster = std::vector<RecordIdentifier>;
using SimInfo = std::map<model::Similarity, std::vector<ValueIdentifier>>;
using SimilarityMatrix = std::vector<std::unordered_map<ValueIdentifier, model::Similarity>>;
using SimilarityIndex = std::vector<SimInfo>;
}