#pragma once

#include <cstddef>
#include <map>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd {
using ValueIdentifier = size_t;

template <typename MetricReturnType>
using MetricResultVector = std::vector<MetricReturnType>;
template <typename MetricReturnType>
using MetricResultMatrix = std::vector<std::unordered_map<ValueIdentifier, MetricReturnType>>;
template <typename MetricReturnType>
using MetricResultIndex = std::vector<std::map<MetricReturnType, std::vector<ValueIdentifier>>>;

using RecordIdentifier = size_t;
using PliCluster = std::vector<RecordIdentifier>;
using DecisionBoundsVector = std::vector<model::Similarity>;
using SimilarityMatrix = std::vector<std::unordered_map<ValueIdentifier, model::Similarity>>;
using SimInfo = std::map<model::Similarity, std::vector<ValueIdentifier>>;
using SimilarityIndex = std::vector<SimInfo>;
using Recommendations = std::vector<std::pair<RecordIdentifier, RecordIdentifier>>;
}  // namespace algos::hymd
