#pragma once
#include <utility>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/similarity.h"
#include "model/index.h"

namespace algos::md {
using RecordsPair = std::pair<hymd::RecordIdentifier, hymd::RecordIdentifier>;
using RecordsSet = boost::unordered_set<hymd::RecordIdentifier>;
using RecordsPairsSet = boost::unordered_map<hymd::RecordIdentifier, RecordsSet>;
using RecordsPairToSimilarityMap = boost::unordered_map<RecordsPair, model::md::Similarity>;

class ViolatingRecordsPairsSet {
private:
    RecordsPairsSet records_pairs_;

public:
    ViolatingRecordsPairsSet() = default;

    ViolatingRecordsPairsSet(RecordsPairsSet records_pairs)
        : records_pairs_(std::move(records_pairs)) {}

    void InsertClusters(hymd::indexes::PliCluster const& left_cluster,
                        hymd::indexes::PliCluster const& right_cluster);
    void InsertPair(hymd::RecordIdentifier left_record, hymd::RecordIdentifier right_record);

    void DeleteClusters(hymd::indexes::PliCluster const& left_cluster,
                        hymd::indexes::PliCluster const& right_cluster);

    void Clear() {
        records_pairs_.clear();
    }

    bool Empty() const {
        return records_pairs_.empty();
    }

    RecordsPairsSet const& GetPairs() const {
        return records_pairs_;
    }
};

}  // namespace algos::md
