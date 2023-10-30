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
using CompressedRecord = std::vector<ValueIdentifier>;

struct Recommendation {
    CompressedRecord const* left_record;
    CompressedRecord const* right_record;

    Recommendation(CompressedRecord const* left_record, CompressedRecord const* right_record)
        : left_record(left_record), right_record(right_record) {}

    friend bool operator==(Recommendation const& a, Recommendation const& b) {
        return *a.left_record == *b.left_record && *a.right_record == *b.right_record;
    }

    friend std::hash<Recommendation>;
};
using Recommendations = std::unordered_set<Recommendation>;

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

// This is probably unneeded, as it is unlikely to have two pairs where all
// values are the same in one dataset, pointer comparison should suffice. If
// there do happen to be pairs like this, they wouldn't cause as much of a
// slowdown as hashing all this does.
template <>
struct hash<algos::hymd::Recommendation> {
    std::size_t operator()(algos::hymd::Recommendation const& p) const noexcept {
        using algos::hymd::CompressedRecord, algos::hymd::ValueIdentifier;
        CompressedRecord const& left_record = *p.left_record;
        CompressedRecord const& right_record = *p.right_record;
        PyTupleHash<ValueIdentifier> hasher{left_record.size() * 2};
        for (ValueIdentifier v : left_record) {
            hasher.AddValue(v);
        }
        for (ValueIdentifier v : right_record) {
            hasher.AddValue(v);
        }
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
