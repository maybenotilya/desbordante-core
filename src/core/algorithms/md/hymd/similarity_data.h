#pragma once

#include <optional>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/indexes/pli_cluster.h"
#include "algorithms/md/hymd/indexes/similarity_index.h"
#include "algorithms/md/hymd/indexes/similarity_matrix.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/similarity_vector.h"
#include "model/index.h"

namespace algos::hymd {

class SimilarityData {
private:
    indexes::CompressedRecords* compressed_records_;
    std::vector<model::md::DecisionBoundary> rhs_min_similarities_;

    std::vector<std::pair<model::Index, model::Index>> column_match_col_indices_;

    std::vector<std::vector<model::md::DecisionBoundary>> natural_decision_bounds_;
    std::vector<preprocessing::Similarity> lowest_sims_;
    std::vector<indexes::SimilarityMatrix> sim_matrices_;
    std::vector<indexes::SimilarityIndex> sim_indexes_;

    bool prune_nondisjoint_ = true;
    bool const single_table_;

    [[nodiscard]] model::Index GetLeftPliIndex(model::Index column_match_index) const;

    indexes::DictionaryCompressor const& GetLeftRecords() const {
        return compressed_records_->GetLeftRecords();
    }

    indexes::DictionaryCompressor const& GetRightRecords() const {
        return compressed_records_->GetRightRecords();
    }

    void LowerForColumnMatch(model::md::DecisionBoundary& threshold, model::Index col_match,
                             indexes::PliCluster const& cluster,
                             std::vector<RecordIdentifier> const& similar_records,
                             std::vector<model::md::DecisionBoundary> const& gen_max_rhs,
                             Recommendations* recommendations_ptr) const;
    void LowerForColumnMatch(model::md::DecisionBoundary& threshold, model::Index col_match,
                             std::vector<CompressedRecord const*> const& cluster,
                             std::vector<RecordIdentifier> const& similar_records,
                             std::vector<model::md::DecisionBoundary> const& gen_max_rhs,
                             Recommendations* recommendations_ptr) const;
    [[nodiscard]] std::vector<RecordIdentifier> const* GetSimilarRecords(
            ValueIdentifier value_id, model::md::DecisionBoundary similarity,
            model::Index column_match_index) const;

public:
    struct LhsData {
        std::vector<model::md::DecisionBoundary> max_rhs_dec_bounds;
        bool is_unsupported;

        LhsData(std::vector<model::md::DecisionBoundary> max_rhs_dec_bounds,
                bool is_unsupported) noexcept
            : max_rhs_dec_bounds(std::move(max_rhs_dec_bounds)), is_unsupported(is_unsupported) {}
    };

    SimilarityData(indexes::CompressedRecords* compressed_records,
                   std::vector<model::md::DecisionBoundary> rhs_min_similarities,
                   std::vector<std::pair<model::Index, model::Index>> column_match_col_indices,
                   std::vector<std::vector<model::md::DecisionBoundary>> natural_decision_bounds,
                   std::vector<preprocessing::Similarity> lowest_sims,
                   std::vector<indexes::SimilarityMatrix> sim_matrices,
                   std::vector<indexes::SimilarityIndex> sim_indexes, bool single_table)
        : compressed_records_(compressed_records),
          rhs_min_similarities_(std::move(rhs_min_similarities)),
          column_match_col_indices_(std::move(column_match_col_indices)),
          natural_decision_bounds_(std::move(natural_decision_bounds)),
          lowest_sims_(std::move(lowest_sims)),
          sim_matrices_(std::move(sim_matrices)),
          sim_indexes_(std::move(sim_indexes)),
          single_table_(single_table) {}

    static std::unique_ptr<SimilarityData> CreateFrom(
            indexes::CompressedRecords* compressed_records,
            std::vector<model::md::DecisionBoundary> min_similarities,
            std::vector<std::pair<model::Index, model::Index>> column_match_col_indices,
            std::vector<preprocessing::similarity_measure::SimilarityMeasure const*> const&
                    sim_measures,
            bool is_null_equal_null);

    [[nodiscard]] std::vector<model::md::DecisionBoundary> const& GetRhsMinSimilarities() const {
        return rhs_min_similarities_;
    }

    [[nodiscard]] size_t GetColumnMatchNumber() const {
        return column_match_col_indices_.size();
    }

    [[nodiscard]] std::pair<model::Index, model::Index> GetColMatchIndices(
            model::Index index) const {
        return column_match_col_indices_[index];
    }

    [[nodiscard]] size_t GetLeftSize() const {
        return GetLeftRecords().GetNumberOfRecords();
    }

    [[nodiscard]] size_t GetRightSize() const {
        return GetRightRecords().GetNumberOfRecords();
    }

    [[nodiscard]] std::optional<model::md::DecisionBoundary> SpecializeOneLhs(
            model::Index col_match_index, model::md::DecisionBoundary similarity) const;

    [[nodiscard]] std::unordered_set<SimilarityVector> GetSimVecs(
            RecordIdentifier left_record_id) const;
    [[nodiscard]] SimilarityVector GetSimilarityVector(CompressedRecord const& left_record,
                                                       CompressedRecord const& right_record) const;

    [[nodiscard]] LhsData GetMaxRhsDecBounds(
            DecisionBoundaryVector const& lhs_sims, Recommendations* recommendations_ptr,
            size_t min_support, DecisionBoundaryVector const& original_rhs_thresholds,
            std::vector<model::md::DecisionBoundary> const& gen_max_rhs,
            std::unordered_set<model::Index>& rhs_indices) const;

    [[nodiscard]] std::optional<model::md::DecisionBoundary> GetPreviousDecisionBound(
            model::md::DecisionBoundary lhs_sim, model::Index col_match) const;
};

}  // namespace algos::hymd
