#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/md/hymd/model/column_match_internal.h"
#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/lattice/lattice.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/md_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/relational_schema.h"

namespace std {
template<>
struct hash<std::pair<size_t, size_t>> {
    std::size_t operator()(std::pair<size_t, size_t> const &p) const {
        auto hasher = std::hash<size_t>{};
        return hasher(p.first) ^ hasher(p.second);
    }
};
}

namespace algos::hymd {

class HyMD final : public MdAlgorithm {
private:
    using ValueIdentifier = size_t;
    using SimSortedRecords = std::vector<size_t>;
    using SimToFirstSimRecMapping = std::unordered_map<model::Similarity, size_t>;
    using SimilarityMatrix = std::vector<std::unordered_map<ValueIdentifier, model::Similarity>>;
    using SimilarityIndex = std::vector<std::pair<SimToFirstSimRecMapping, SimSortedRecords>>;

    config::InputTable left_table_;
    config::InputTable right_table_;

    std::unique_ptr<RelationalSchema> left_schema_;
    std::unique_ptr<RelationalSchema> right_schema_;

    model::DictionaryCompressor records_left_;
    model::DictionaryCompressor records_right_;

    std::vector<model::ColumnMatchInternal> column_matches_;

    std::vector<SimilarityMatrix> sim_matrices_;
    // col_match_sim_index = sim_index_[column_match.left_col_index]
    // col_match_sim_index[left_record_id].second[>=sim_index_[left_record_id].first[similarity]]
    std::vector<SimilarityIndex> sim_indexes_;

    model::Lattice lattice_;
    size_t cur_level_ = 0;
    size_t min_support_ = 0;

    size_t cur_record_left_ = 0;
    size_t cur_record_right_ = 0;
    std::unordered_set<std::pair<size_t, size_t>> recommendations_;
    std::unordered_set<std::pair<size_t, size_t>> checked_recommendations_;

    void ResetStateMd() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

    bool TraverseLattice(bool traverse_all);
    bool InferFromRecordPairs();

    void FillSimilarities();
    // Needs cardinality, consider a separate class for LHS.
    std::pair<std::vector<double>, size_t> GetMaxRhsDecBounds(
            model::SimilarityVector const& lhs_sims);
    void MarkUnsupported(model::SimilarityVector const& lhs);
    bool CanSpecializeLhs(model::SimilarityVector const& lhs, size_t col_match_index);
    model::SimilarityVector SpecializeLhs(model::SimilarityVector const& lhs,
                                          size_t col_match_index);
    bool IsSupported(model::SimilarityVector const& sim_vec);
};

}  // namespace algos
