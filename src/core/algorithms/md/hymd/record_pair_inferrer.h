#pragma once

#include <list>
#include <unordered_set>

#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/similarity_vector.h"

namespace algos::hymd {

class RecordPairInferrer {
private:
    struct Statistics {
        size_t samples_done = 0;
        size_t sim_vecs_processed = 0;
    };

    SimilarityData* similarity_data_;

    lattice::FullLattice* lattice_;
    Recommendations* recommendations_ptr_;

    // Metanome uses a linked list for some reason.
    std::unordered_set<SimilarityVector> sim_vecs_to_check_;
    std::unordered_set<SimilarityVector> checked_sim_vecs_;

    RecordIdentifier cur_record_left_ = 0;

    // size_t efficiency_reciprocal_ = 100;

    bool const prune_nondisjoint_ = true;
    bool const avoid_same_sim_vec_processing_ = true;

    void ProcessSimVec(SimilarityVector const& sim);
    bool ShouldKeepInferring(Statistics const& statistics) const;

public:
    RecordPairInferrer(SimilarityData* similarity_data, lattice::FullLattice* lattice,
                       Recommendations* recommendations_ptr)
        : similarity_data_(similarity_data),
          lattice_(lattice),
          recommendations_ptr_(recommendations_ptr) {}

    bool InferFromRecordPairs();
};

}  // namespace algos::hymd
