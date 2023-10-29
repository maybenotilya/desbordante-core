#pragma once

#include <list>

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

    // Metanome uses a linked list for some reason.
    DecBoundVectorUnorderedSet sim_vecs_to_check_;
    DecBoundVectorUnorderedSet checked_sim_vecs_;

    size_t cur_record_left_ = 0;
    size_t cur_record_right_ = 0;

    size_t efficiency_reciprocal_ = 100;

    bool const prune_nondisjoint_ = true;
    bool const avoid_same_sim_vec_processing_ = false;

    size_t CheckRecordPair(size_t left_record, size_t right_record);
    size_t ProcessSimVec(DecisionBoundsVector const& sim);
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