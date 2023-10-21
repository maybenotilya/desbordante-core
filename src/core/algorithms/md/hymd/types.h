#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
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
using SimilarityMatrix =
        std::unordered_map<ValueIdentifier, std::unordered_map<ValueIdentifier, model::Similarity>>;
using SimInfo = std::map<model::Similarity, std::vector<RecordIdentifier>>;
using SimilarityIndex = std::unordered_map<ValueIdentifier, SimInfo>;
using Recommendations = std::vector<std::pair<RecordIdentifier, RecordIdentifier>>;

using SimilarityFunction =
        std::function<std::unique_ptr<std::byte const>(std::byte const*, std::byte const*)>;
}  // namespace algos::hymd
