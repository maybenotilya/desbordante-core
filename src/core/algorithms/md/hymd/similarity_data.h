#pragma once

#include <optional>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/indexes/pli_cluster.h"
#include "algorithms/md/hymd/indexes/similarity_index.h"
#include "algorithms/md/hymd/indexes/similarity_matrix.h"
#include "algorithms/md/hymd/lattice/full_lattice.h"
#include "algorithms/md/hymd/lattice/validation_info.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "algorithms/md/hymd/recommendation.h"
#include "algorithms/md/hymd/similarity_vector.h"
#include "model/index.h"

namespace algos::hymd {

class SimilarityData {
private:
    struct WorkingInfo {
        std::vector<Recommendation>& violations;
        model::md::DecisionBoundary const old_bound;
        model::Index const index;
        model::md::DecisionBoundary& threshold;
        size_t const col_match_values;
        model::md::DecisionBoundary interestingness_boundary;
        std::vector<CompressedRecord> const& right_records;
        indexes::SimilarityMatrix const& sim_matrix;

        void SetOld() {
            threshold = old_bound;
        }

        bool EnoughViolations() const {
            return violations.size() >= 20;
        }

        bool ShouldStop() const {
            return threshold == 0.0 && EnoughViolations();
        }

        WorkingInfo(model::md::DecisionBoundary old_bound, model::Index index,
                    std::vector<Recommendation>& violations, model::md::DecisionBoundary& threshold,
                    size_t col_match_values, std::vector<CompressedRecord> const& right_records,
                    indexes::SimilarityMatrix const& sim_matrix)
            : violations(violations),
              old_bound(old_bound),
              index(index),
              threshold(threshold),
              col_match_values(col_match_values),
              right_records(right_records),
              sim_matrix(sim_matrix) {}
    };
    template <typename Func>
    static auto ZeroWorking(std::vector<WorkingInfo>& working_info, Func func);

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

    size_t GetLeftValueNum(model::Index col_match_index) const {
        return GetLeftRecords().GetPli(GetLeftPliIndex(col_match_index)).GetClusters().size();
    }

    [[nodiscard]] bool LowerForColumnMatch(WorkingInfo& working_info, indexes::PliCluster const& cluster,
                             std::unordered_set<RecordIdentifier> const& similar_records) const;
    [[nodiscard]] bool LowerForColumnMatch(WorkingInfo& working_info,
                             std::vector<CompressedRecord const*> const& cluster,
                             std::unordered_set<RecordIdentifier> const& similar_records) const;
    [[nodiscard]] std::unordered_set<RecordIdentifier> const* GetSimilarRecords(
            ValueIdentifier value_id, model::md::DecisionBoundary similarity,
            model::Index column_match_index) const;

public:
    struct ValidationResult {
        std::vector<std::vector<Recommendation>> violations;
        std::vector<std::pair<model::Index, model::md::DecisionBoundary>> to_specialize;
        bool is_unsupported;
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

    [[nodiscard]] ValidationResult Validate(lattice::FullLattice& lattice,
                                            lattice::ValidationInfo* validation_info,
                                            size_t min_support) const;

    [[nodiscard]] std::optional<model::md::DecisionBoundary> GetPreviousDecisionBound(
            model::md::DecisionBoundary lhs_sim, model::Index col_match) const;
};

}  // namespace algos::hymd
