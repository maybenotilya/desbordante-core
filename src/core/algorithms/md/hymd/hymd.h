#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/md/hymd/model/column_match_internal.h"
#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/lattice/lattice.h"
#include "algorithms/md/md_algorithm.h"
#include "config/tabular_data/input_table_type.h"

namespace algos::hymd {

class HyMD final : public MdAlgorithm {
private:
    using Similarity = double;
    using ValueIdentifier = size_t;
    using SimSortedRecords = std::vector<size_t>;
    using SimToFirstSimRecMapping = std::unordered_map<Similarity, size_t>;
    using SimilarityMatrix = std::vector<std::unordered_map<ValueIdentifier, Similarity>>;
    using SimilarityIndex = std::vector<std::pair<SimToFirstSimRecMapping, SimSortedRecords>>;

    config::InputTable left_table_;
    config::InputTable right_table_;

    model::DictionaryCompressor records_left_;
    model::DictionaryCompressor records_right_;

    std::vector<model::ColumnMatchInternal> column_matches_;

    std::vector<SimilarityMatrix> sim_matrices_;
    // col_match_sim_index = sim_index_[column_match.left_col_index]
    // col_match_sim_index[left_record_id].second[>=sim_index_[left_record_id].first[similarity]]
    std::vector<SimilarityIndex> sim_indexes_;

    model::Lattice lattice_;

    void ResetStateMd() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

    bool TraverseLattice(bool traverse_all);
    bool InferFromRecordPairs();

    void FillSimilarities();
    // Needs cardinality, consider a separate class for LHS.
    std::pair<std::vector<double>, size_t> GetMaxRhsDecBounds(std::vector<double> lhs_sims);
};

}  // namespace algos
