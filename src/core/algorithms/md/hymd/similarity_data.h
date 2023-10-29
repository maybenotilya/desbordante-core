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
    bool const single_table_;

    [[nodiscard]] std::map<size_t, std::vector<size_t>> MakeColToColMatchMapping(
            std::vector<size_t> const& col_match_indices) const;
    [[nodiscard]] size_t GetLeftPliIndex(size_t column_match_index) const;

    model::DictionaryCompressor const& GetLeftRecords() const {
        return compressed_records_->GetLeftRecords();
    }

    model::DictionaryCompressor const& GetRightRecords() const {
        return compressed_records_->GetRightRecords();
    }

    void DecreaseRhsThresholds(model::SimilarityVector& rhs_thresholds, PliCluster const& cluster,
                               std::vector<size_t> const& similar_records,
                               model::SimilarityVector const& gen_max_rhs,
                               Recommendations* recommendations_ptr) const;
    void LowerForColumnMatch(double& threshold, size_t col_match, PliCluster const& cluster,
                             std::vector<size_t> const& similar_records,
                             model::SimilarityVector const& gen_max_rhs,
                             Recommendations* recommendations_ptr) const;
    [[nodiscard]] std::vector<RecordIdentifier> GetSimilarRecords(ValueIdentifier value_id,
                                                                  model::Similarity similarity,
                                                                  size_t column_match_index) const;

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
                   std::vector<SimilarityIndex> sim_indexes,
                   bool single_table)
        : compressed_records_(compressed_records),
          rhs_min_similarities_(std::move(rhs_min_similarities)),
          column_match_col_indices_(std::move(column_match_col_indices)),
          natural_decision_bounds_(std::move(natural_decision_bounds)),
          lowest_sims_(std::move(lowest_sims)),
          sim_matrices_(std::move(sim_matrices)),
          sim_indexes_(std::move(sim_indexes)),
          single_table_(single_table) {}

    static std::unique_ptr<SimilarityData> CreateFrom(
            model::CompressedRecords* compressed_records, model::SimilarityVector min_similarities,
            std::vector<std::pair<size_t, size_t>> column_match_col_indices,
            std::vector<model::SimilarityMeasure const*> const& sim_measures,
            bool is_null_equal_null);

    [[nodiscard]] model::SimilarityVector const& GetRhsMinSimilarities() const {
        return rhs_min_similarities_;
    }

    [[nodiscard]] size_t GetColumnMatchNumber() const {
        return column_match_col_indices_.size();
    }

    [[nodiscard]] std::pair<size_t, size_t> GetColMatchIndices(size_t index) const {
        return column_match_col_indices_[index];
    }

    [[nodiscard]] size_t GetLeftSize() const {
        return compressed_records_->GetLeftRecords().GetNumberOfRecords();
    }

    [[nodiscard]] size_t GetRightSize() const {
        return compressed_records_->GetRightRecords().GetNumberOfRecords();
    }

    [[nodiscard]] std::optional<model::Similarity> SpecializeOneLhs(
            size_t col_match_index, model::Similarity similarity) const;
    [[nodiscard]] std::optional<model::SimilarityVector> SpecializeLhs(
            model::SimilarityVector const& lhs, size_t col_match_index) const;
    [[nodiscard]] std::optional<model::SimilarityVector> SpecializeLhs(
            model::SimilarityVector const& lhs, size_t col_match_index,
            model::Similarity similarity) const;

    [[nodiscard]] DecBoundVectorUnorderedSet GetSimVecs(RecordIdentifier left_record) const;
    [[nodiscard]] model::SimilarityVector GetSimilarityVector(size_t left_record,
                                                              size_t right_record) const;

    [[nodiscard]] LhsData GetMaxRhsDecBounds(model::SimilarityVector const& lhs_sims,
                                             Recommendations* recommendations_ptr,
                                             size_t min_support,
                                             model::SimilarityVector rhs_thresholds,
                                             model::SimilarityVector const& gen_max_rhs) const;

    [[nodiscard]] std::optional<model::Similarity> GetPreviousSimilarity(model::Similarity lhs_sim,
                                                                         size_t col_match) const;
};

}  // namespace algos::hymd
