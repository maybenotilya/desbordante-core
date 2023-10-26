#pragma once

#include <optional>
#include <vector>

#include "algorithms/md/hymd/model/compressed_records.h"
#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"
#include "algorithms/md/hymd/model/similarity_measure/similarity_measure.h"
#include "algorithms/md/hymd/types.h"

namespace algos::hymd {

class SimilarityData {
private:
    model::CompressedRecords* compressed_records_;
    model::SimilarityVector rhs_min_similarities_;

    std::vector<std::pair<size_t, size_t>> column_match_col_indices_;

    std::vector<std::vector<model::Similarity>> natural_decision_bounds_;
    std::vector<model::Similarity> lowest_sims_;
    std::vector<SimilarityMatrix> sim_matrices_;
    std::vector<SimilarityIndex> sim_indexes_;

    bool prune_nondisjoint_ = true;

    [[nodiscard]] std::map<size_t, std::vector<size_t>> MakeColToColMatchMapping(
            std::vector<size_t> const& col_match_indices) const;
    [[nodiscard]] size_t GetLeftPliIndex(size_t column_match_index) const;

    void DecreaseRhsThresholds(model::SimilarityVector& rhs_thresholds, PliCluster const& cluster,
                               std::vector<size_t> const& similar_records, model::SimilarityVector const& gen_max_rhs,
                               Recommendations* recommendations_ptr) const;
    [[nodiscard]] std::vector<RecordIdentifier> GetSimilarRecords(ValueIdentifier value_id,
                                                                  model::Similarity similarity,
                                                                  size_t column_match_index) const;
    void LowerForColumnMatch(double& threshold, size_t col_match, PliCluster const& cluster,
                             std::vector<size_t> const& similar_records,
                             model::SimilarityVector const& gen_max_rhs,
                             Recommendations* recommendations_ptr) const;

public:
    struct LhsData {
        model::SimilarityVector max_rhs_dec_bounds;
        size_t support;

        LhsData(model::SimilarityVector max_rhs_dec_bounds, size_t support) noexcept
            : max_rhs_dec_bounds(std::move(max_rhs_dec_bounds)), support(support) {}
    };

    SimilarityData(model::CompressedRecords* compressed_records,
                   model::SimilarityVector rhs_min_similarities,
                   std::vector<std::pair<size_t, size_t>> column_match_col_indices,
                   std::vector<DecisionBoundsVector> natural_decision_bounds,
                   std::vector<model::Similarity> lowest_sims,
                   std::vector<SimilarityMatrix> sim_matrices,
                   std::vector<SimilarityIndex> sim_indexes)
        : compressed_records_(compressed_records),
          rhs_min_similarities_(std::move(rhs_min_similarities)),
          column_match_col_indices_(std::move(column_match_col_indices)),
          natural_decision_bounds_(std::move(natural_decision_bounds)),
          lowest_sims_(std::move(lowest_sims)),
          sim_matrices_(std::move(sim_matrices)),
          sim_indexes_(std::move(sim_indexes)) {}

    static std::unique_ptr<SimilarityData> CreateFrom(
            model::CompressedRecords* compressed_records, model::SimilarityVector min_similarities,
            std::vector<std::pair<size_t, size_t>> column_match_col_indices,
            std::vector<model::SimilarityMeasure const*> const& sim_measures,
            bool is_null_equal_null);

    [[nodiscard]] std::vector<std::vector<model::Similarity>> const& GetNaturalDecisionBounds()
            const {
        return natural_decision_bounds_;
    }

    [[nodiscard]] std::vector<SimilarityMatrix> const& GetSimilarityMatrices() const {
        return sim_matrices_;
    }

    [[nodiscard]] std::vector<SimilarityIndex> const& GetSimilarityIndexes() const {
        return sim_indexes_;
    }

    [[nodiscard]] model::SimilarityVector const& GetRhsMinSimilarities() const {
        return rhs_min_similarities_;
    }

    [[nodiscard]] size_t GetColumnMatchNumber() const {
        return column_match_col_indices_.size();
    }

    [[nodiscard]] std::pair<size_t, size_t> GetColMatchIndices(size_t index) const {
        return column_match_col_indices_[index];
    }

    [[nodiscard]] model::DictionaryCompressor const& GetLeftRecords() const {
        return compressed_records_->GetLeftRecords();
    }

    [[nodiscard]] model::DictionaryCompressor const& GetRightRecords() const {
        return compressed_records_->GetRightRecords();
    }

    [[nodiscard]] std::optional<model::SimilarityVector> SpecializeLhs(
            model::SimilarityVector const& lhs, size_t col_match_index) const;
    [[nodiscard]] std::optional<model::SimilarityVector> SpecializeLhs(
            model::SimilarityVector const& lhs, size_t col_match_index,
            model::Similarity similarity) const;

    [[nodiscard]] model::SimilarityVector GetSimilarityVector(size_t left_record,
                                                              size_t right_record) const;

    [[nodiscard]] LhsData GetMaxRhsDecBounds(model::SimilarityVector const& lhs_sims,
                                             Recommendations* recommendations_ptr,
                                             size_t min_support,
                                             model::SimilarityVector rhs_thresholds, model::SimilarityVector const& gen_max_rhs) const;
};

}  // namespace algos::hymd
