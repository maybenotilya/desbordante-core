#pragma once

#include <cstddef>
#include <functional>
#include <unordered_set>

#include "algorithms/md/hymd/compressed_record.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "util/py_tuple_hash.h"

namespace algos::hymd {
struct Recommendation {
    CompressedRecord const* left_record;
    CompressedRecord const* right_record;

    Recommendation(CompressedRecord const* left_record, CompressedRecord const* right_record)
        : left_record(left_record), right_record(right_record) {}

    friend bool operator==(Recommendation const& a, Recommendation const& b) {
        return *a.left_record == *b.left_record && *a.right_record == *b.right_record;
    }
};

using Recommendations = std::unordered_set<Recommendation>;
}  // namespace algos::hymd

namespace std {
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
        util::PyTupleHash<ValueIdentifier> hasher{left_record.size() * 2};
        for (ValueIdentifier v : left_record) {
            hasher.AddValue(v);
        }
        for (ValueIdentifier v : right_record) {
            hasher.AddValue(v);
        }
        return hasher.GetResult();
    }
};
}  // namespace std
