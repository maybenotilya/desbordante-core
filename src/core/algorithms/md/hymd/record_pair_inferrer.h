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
    struct Statistics;

    SimilarityData* similarity_data_;

    lattice::FullLattice* lattice_;

    // Metanome uses a linked list for some reason.
    std::unordered_set<SimilarityVector> sim_vecs_to_check_;
    std::unordered_set<SimilarityVector> checked_sim_vecs_;

    RecordIdentifier next_left_record_ = 0;

    // std::size_t efficiency_reciprocal_ = 100;

    bool const prune_nondisjoint_ = true;
    bool const avoid_same_sim_vec_processing_ = true;

    void ProcessSimVec(SimilarityVector const& sim);
    bool ShouldStopInferring(Statistics const& statistics) const;

public:
    RecordPairInferrer(SimilarityData* similarity_data, lattice::FullLattice* lattice)
        : similarity_data_(similarity_data), lattice_(lattice) {}

    bool InferFromRecordPairs(Recommendations recommendations);
};

}  // namespace algos::hymd
