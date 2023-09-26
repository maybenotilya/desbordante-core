#pragma once

#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/similarity_info.h"

namespace algos::hymd {

class RecordPairInferrer {
private:
    model::DictionaryCompressor* records_left_;
    model::DictionaryCompressor* records_right_;

    model::SimilarityVector* rhs_min_similarities_;
    model::MdLattice* md_lattice_;

    size_t cur_record_left_ = 0;
    size_t cur_record_right_ = 0;
    std::vector<std::pair<size_t, size_t>> recommendations_;
    std::unordered_set<std::pair<size_t, size_t>> checked_recommendations_;

    size_t efficiency_reciprocal_ = 100;

    size_t CheckRecordPair(size_t left_record, size_t right_record);
    bool ShouldKeepInferring(size_t records_checked, size_t mds_refined) const;

public:
    bool InferFromRecordPairs();
};

}  // namespace algos::hymd
