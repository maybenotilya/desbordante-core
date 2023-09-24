#pragma once

#include <optional>
#include <set>

#include "algorithms/algorithm.h"
#include "algorithms/md/column_match.h"
#include "algorithms/md/hymd/model/column_match_internal.h"
#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"
#include "algorithms/md/hymd/model/min_picker_lattice/min_picker_lattice.h"
#include "algorithms/md/hymd/model/similarity.h"
#include "algorithms/md/hymd/model/support_lattice/support_lattice.h"
#include "algorithms/md/md_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/relational_schema.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace std {
template <>
struct hash<std::pair<size_t, size_t>> {
    std::size_t operator()(std::pair<size_t, size_t> const& p) const {
        auto hasher = std::hash<size_t>{};
        return hasher(p.first) ^ hasher(p.second);
    }
};
}  // namespace std

namespace algos::hymd {

class HyMD final : public MdAlgorithm {
private:
    using ValueIdentifier = size_t;
    using RecordIdentifier = size_t;
    using PliCluster = std::vector<RecordIdentifier>;
    using SimInfo = std::map<model::Similarity, std::vector<ValueIdentifier>>;
    using SimilarityMatrix = std::vector<std::unordered_map<ValueIdentifier, model::Similarity>>;
    using SimilarityIndex = std::vector<SimInfo>;

    config::InputTable left_table_;
    config::InputTable right_table_;

    std::unique_ptr<RelationalSchema> left_schema_;
    std::unique_ptr<RelationalSchema> right_schema_;

    std::unique_ptr<model::DictionaryCompressor> records_left_;
    std::unique_ptr<model::DictionaryCompressor> records_right_;

    bool is_null_equal_null_ = true;
    std::unique_ptr<::model::ColumnLayoutTypedRelationData> typed_relation_data_left_;
    std::unique_ptr<::model::ColumnLayoutTypedRelationData> typed_relation_data_right_;

    std::vector<std::tuple<std::string, std::string, std::string>> column_matches_option_;
    std::vector<model::ColumnMatchInternal> column_matches_;
    model::SimilarityVector rhs_min_similarities_;

    std::vector<std::vector<model::Similarity>> natural_decision_bounds_;

    std::vector<SimilarityMatrix> sim_matrices_;
    // col_match_sim_index = sim_index_[column_match.left_col_index]
    // col_match_sim_index[left_record_id].second[>=sim_index_[left_record_id].first[similarity]]
    std::vector<SimilarityIndex> sim_indexes_;

    std::unique_ptr<model::MdLattice> md_lattice_;
    size_t cur_level_ = 0;
    std::unique_ptr<model::SupportLattice> support_lattice_;
    size_t min_support_ = 0;
    std::unique_ptr<model::MinPickerLattice> min_picker_lattice_;

    size_t cur_record_left_ = 0;
    size_t cur_record_right_ = 0;
    std::vector<std::pair<size_t, size_t>> recommendations_;
    std::unordered_set<std::pair<size_t, size_t>> checked_recommendations_;

    size_t efficiency_reciprocal_ = 100;

    void ResetStateMd() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

    bool TraverseLattice(bool traverse_all);
    bool InferFromRecordPairs();

    void FillSimilarities();

    std::optional<model::SimilarityVector> SpecializeLhs(model::SimilarityVector const& lhs,
                                                         size_t col_match_index);

    size_t CheckRecordPair(size_t left_record, size_t right_record);
    bool ShouldKeepInferring(size_t records_checked, size_t mds_refined) const;
    model::SimilarityVector GetSimilarityVector(size_t left_record, size_t right_record);
    std::optional<model::SimilarityVector> SpecializeLhs(model::SimilarityVector const& lhs,
                                                         size_t col_match_index,
                                                         model::Similarity similarity);

    // Needs cardinality
    // AKA `Validate`
    std::pair<model::SimilarityVector, size_t> GetMaxRhsDecBounds(
            model::SimilarityVector const& lhs_sims);
    static std::vector<RecordIdentifier> GetSimilarRecords(ValueIdentifier value_id,
                                                           model::Similarity similarity,
                                                           SimilarityIndex const& sim_index);
    void DecreaseRhsThresholds(model::SimilarityVector& rhs_thresholds, PliCluster const& cluster,
                               std::vector<size_t> const& similar_records);
    std::map<size_t, std::vector<size_t>> MakeColToColMatchMapping(
            std::vector<size_t> const& col_match_indices);
    size_t GetPliIndex(size_t column_match_index);

    void RegisterResults();

public:
    HyMD();
};

}  // namespace algos::hymd
