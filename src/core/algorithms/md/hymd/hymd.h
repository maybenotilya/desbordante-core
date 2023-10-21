#pragma once

#include <optional>
#include <set>

#include "algorithms/algorithm.h"
#include "algorithms/md/hymd/lattice_traverser.h"
#include "algorithms/md/hymd/model/compressed_records.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/similarity_measure/similarity_measure.h"
#include "algorithms/md/hymd/record_pair_inferrer.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/types.h"
#include "algorithms/md/md_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/relational_schema.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::hymd {

class HyMD final : public MdAlgorithm {
private:
    config::InputTable left_table_;
    config::InputTable right_table_;

    std::unique_ptr<RelationalSchema> left_schema_;
    std::unique_ptr<RelationalSchema> right_schema_;

    std::unique_ptr<model::CompressedRecords> compressed_records_;

    // shared_ptr(?)
    std::unique_ptr<SimilarityData> similarity_data_;
    std::unique_ptr<model::FullLattice> lattice_;
    std::vector<std::pair<size_t, size_t>> recommendations_;

    std::unique_ptr<LatticeTraverser> lattice_traverser_;
    std::unique_ptr<RecordPairInferrer> record_pair_inferrer_;

    bool is_null_equal_null_ = true;
    size_t min_support_ = 0;

    // ??: need different types of sim measures, need to have
    std::vector<model::SimilarityMeasure*> sim_measures_;
    std::vector<std::tuple<std::string, std::string, std::unique_ptr<model::SimilarityMeasure>>>
            column_matches_option_;
    model::SimilarityVector rhs_min_similarities_;

    void ResetStateMd() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

    void RegisterResults();

public:
    HyMD();
};

}  // namespace algos::hymd
