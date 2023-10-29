#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithms/md/hymd/model/similarity.h"
#include "util/custom_hashes.h"

namespace algos::hymd {
using ValueIdentifier = size_t;

using RecordIdentifier = size_t;
using PliCluster = std::vector<RecordIdentifier>;
using DecisionBoundsVector = std::vector<model::Similarity>;
using DecBoundVectorUnorderedSet = std::unordered_set<DecisionBoundsVector>;
using SimilarityMatrixRow = std::unordered_map<ValueIdentifier, model::Similarity>;
using SimilarityMatrix = std::unordered_map<ValueIdentifier, SimilarityMatrixRow>;
using SimInfo = std::map<model::Similarity, std::vector<RecordIdentifier>>;
using SimilarityIndex = std::unordered_map<ValueIdentifier, SimInfo>;
using Recommendations = std::unordered_set<std::pair<RecordIdentifier, RecordIdentifier>>;
using CompressedRecord = std::vector<ValueIdentifier>;

using SimilarityFunction =
        std::function<std::unique_ptr<std::byte const>(std::byte const*, std::byte const*)>;
}  // namespace algos::hymd

namespace std {
template <>
struct hash<std::pair<size_t, size_t>> {
    std::size_t operator()(std::pair<size_t, size_t> const& p) const noexcept {
        PyTupleHash<size_t> hasher{2};
        hasher.AddValue(p.first);
        hasher.AddValue(p.second);
        return hasher.GetResult();
    }
};

template <>
struct hash<algos::hymd::DecisionBoundsVector> {
    std::size_t operator()(algos::hymd::DecisionBoundsVector const& p) const noexcept {
        PyTupleHash<double> hasher{p.size()};
        for (double el : p) {
            hasher.AddValue(el);
        }
        return hasher.GetResult();
    }
};
}  // namespace std
