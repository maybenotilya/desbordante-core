#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/model/similarity.h"

namespace std {
template <>
struct hash<std::pair<size_t, size_t>> {
    std::size_t operator()(std::pair<size_t, size_t> const& p) const {
        auto hasher = std::hash<size_t>{};
        return hasher(p.first) ^ hasher(p.second);
    }
};
}  // namespace std

namespace algos::hymd {
using ValueIdentifier = size_t;

using RecordIdentifier = size_t;
using PliCluster = std::vector<RecordIdentifier>;
using DecisionBoundsVector = std::vector<model::Similarity>;
using SimilarityMatrix =
        std::unordered_map<ValueIdentifier, std::unordered_map<ValueIdentifier, model::Similarity>>;
using SimInfo = std::map<model::Similarity, std::vector<RecordIdentifier>>;
using SimilarityIndex = std::unordered_map<ValueIdentifier, SimInfo>;
using Recommendations = std::unordered_set<std::pair<RecordIdentifier, RecordIdentifier>>;
using CompressedRecord = std::vector<ValueIdentifier>;

using SimilarityFunction =
        std::function<std::unique_ptr<std::byte const>(std::byte const*, std::byte const*)>;
}  // namespace algos::hymd
