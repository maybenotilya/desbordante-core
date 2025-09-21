#include "algorithms/md/md_verifier/validation/records_pairs.h"

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/index_range.h"

namespace algos::md {
void ViolatingRecordsPairsSet::InsertClusters(hymd::indexes::PliCluster const& left_cluster,
                                              hymd::indexes::PliCluster const& right_cluster) {
    for (hymd::RecordIdentifier left_record : left_cluster) {
        records_pairs_[left_record].insert(right_cluster.begin(), right_cluster.end());
    }
}

void ViolatingRecordsPairsSet::InsertPair(hymd::RecordIdentifier left_record,
                                          hymd::RecordIdentifier right_record) {
    records_pairs_[left_record].insert(right_record);
};

void ViolatingRecordsPairsSet::DeleteClusters(hymd::indexes::PliCluster const& left_cluster,
                                              hymd::indexes::PliCluster const& right_cluster) {
    for (hymd::RecordIdentifier left_record : left_cluster) {
        auto it = records_pairs_.find(left_record);
        if (it == records_pairs_.end()) {
            continue;
        }
        RecordsSet& valid_right_records = it->second;

        for (hymd::RecordIdentifier right_record : right_cluster) {
            valid_right_records.erase(right_record);
            if (valid_right_records.empty()) {
                records_pairs_.erase(it);
                break;
            }
        }
    }
}

}  // namespace algos::md
