#pragma once

#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "md/hymd/model/full_lattice.h"

namespace algos::hymd {

class RecordPairInferrer {
private:
    SimilarityData* similarity_data_;

    model::FullLattice* lattice_;
    Recommendations* recommendations_ptr_;

    std::unordered_set<std::pair<size_t, size_t>> checked_recommendations_;

    size_t cur_record_left_ = 0;
    size_t cur_record_right_ = 0;

    size_t efficiency_reciprocal_ = 100;

    size_t CheckRecordPair(size_t left_record, size_t right_record, bool prune_nondisjoint = true);
    bool ShouldKeepInferring(size_t records_checked, size_t mds_refined) const;

public:
    RecordPairInferrer(SimilarityData* similarity_data, model::FullLattice* lattice,
                       Recommendations* recommendations_ptr)
        : similarity_data_(similarity_data),
          lattice_(lattice),
          recommendations_ptr_(recommendations_ptr) {}

    bool InferFromRecordPairs();
};

}  // namespace algos::hymd
