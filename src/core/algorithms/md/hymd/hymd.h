#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/md/hymd/model/column_match_internal.h"
#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/lattice/lattice.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/md_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/relational_schema.h"

#include <optional>

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
    using SimSortedInfo = std::vector<std::pair<model::Similarity, ValueIdentifier>>;
    using SimilarityMatrix = std::vector<std::unordered_map<ValueIdentifier, model::Similarity>>;
    using SimilarityIndex = std::vector<SimSortedInfo>;

    config::InputTable left_table_;
    config::InputTable right_table_;

    std::unique_ptr<RelationalSchema> left_schema_;
    std::unique_ptr<RelationalSchema> right_schema_;

    model::DictionaryCompressor records_left_;
    model::DictionaryCompressor records_right_;

    std::vector<model::ColumnMatchInternal> column_matches_;
    model::SimilarityVector rhs_min_similarities_;

    std::vector<SimilarityMatrix> sim_matrices_;
    // col_match_sim_index = sim_index_[column_match.left_col_index]
    // col_match_sim_index[left_record_id].second[>=sim_index_[left_record_id].first[similarity]]
    std::vector<SimilarityIndex> sim_indexes_;

    model::Lattice lattice_;
    size_t cur_level_ = 0;
    size_t min_support_ = 0;

    size_t cur_record_left_ = 0;
    size_t cur_record_right_ = 0;
    std::vector<std::pair<size_t, size_t>> recommendations_;
    std::unordered_set<std::pair<size_t, size_t>> checked_recommendations_;

    size_t inference_run_records_checked_ = 0;
    size_t inference_run_mds_refined_ = 0;
    size_t efficiency_reciprocal_ = 100;

    void ResetStateMd() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

    bool TraverseLattice(bool traverse_all);
    bool InferFromRecordPairs();

    void FillSimilarities();

    void MarkUnsupported(model::SimilarityVector const& lhs);
    bool IsSupported(model::SimilarityVector const& sim_vec);

    std::optional<model::SimilarityVector> SpecializeLhs(model::SimilarityVector const& lhs,
                                                         size_t col_match_index);

    bool CheckRecordPair(size_t left_record, size_t right_record);
    model::SimilarityVector GetSimilarityVector(size_t left_record, size_t right_record);
    std::optional<model::SimilarityVector> SpecializeLhs(model::SimilarityVector const& lhs,
                                                         size_t col_match_index,
                                                         model::Similarity similarity);
};

}  // namespace algos
