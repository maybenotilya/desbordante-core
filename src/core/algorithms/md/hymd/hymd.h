#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <set>

#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice_traverser.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "algorithms/md/hymd/record_pair_inferrer.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"
#include "algorithms/md/md_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/relational_schema.h"

namespace algos::hymd {

class HyMD final : public MdAlgorithm {
private:
    config::InputTable left_table_;
    config::InputTable right_table_;

    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;

    std::unique_ptr<indexes::CompressedRecords> compressed_records_;

    // shared_ptr(?)
    std::unique_ptr<SimilarityData> similarity_data_;
    std::unique_ptr<lattice::FullLattice> lattice_;

    std::unique_ptr<LatticeTraverser> lattice_traverser_;
    std::unique_ptr<RecordPairInferrer> record_pair_inferrer_;

    std::size_t min_support_ = 0;

    std::vector<std::shared_ptr<SimilarityMeasureCreator>> column_matches_option_;

    void ResetStateMd() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

    void RegisterResults();

public:
    HyMD();
};

}  // namespace algos::hymd
