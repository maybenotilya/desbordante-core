#pragma once

#include <optional>
#include <vector>

#include "algorithms/md/hymd/types.h"
#include "algorithms/md/hymd/model/column_match_internal.h"

namespace algos::hymd {

class SimilarityData {
private:
    std::vector<std::vector<model::Similarity>> natural_decision_bounds_;

    std::vector<SimilarityMatrix> sim_matrices_;
    std::vector<SimilarityIndex> sim_indexes_;

    std::vector<model::ColumnMatchInternal>* column_matches_;

    std::map<size_t, std::vector<size_t>> MakeColToColMatchMapping(
            std::vector<size_t> const& col_match_indices) const;
    size_t GetPliIndex(size_t column_match_index) const;

    void DecreaseRhsThresholds(model::SimilarityVector& rhs_thresholds, PliCluster const& cluster,
                               std::vector<size_t> const& similar_records) const;
    std::vector<RecordIdentifier> GetSimilarRecords(ValueIdentifier value_id,
                                                    model::Similarity similarity,
                                                    size_t column_match_index) const;

public:
    SimilarityData(std::vector<std::vector<model::Similarity>> natural_decision_bounds,
                   std::vector<SimilarityMatrix> sim_matrices,
                   std::vector<SimilarityIndex> sim_indexes,
                   std::vector<model::ColumnMatchInternal>* column_matches)
        : natural_decision_bounds_(std::move(natural_decision_bounds)),
          sim_matrices_(std::move(sim_matrices)),
          sim_indexes_(std::move(sim_indexes)),
          column_matches_(column_matches) {}

    std::vector<std::vector<model::Similarity>> const& GetNaturalDecisionBounds() const {
        return natural_decision_bounds_;
    }

    std::vector<SimilarityMatrix> const& GetSimilarityMatrices() const {
        return sim_matrices_;
    }

    std::vector<SimilarityIndex> const& GetSimilarityIndexes() const {
        return sim_indexes_;
    }

    std::optional<model::SimilarityVector> SpecializeLhs(model::SimilarityVector const& lhs,
                                                         size_t col_match_index) const;
    std::optional<model::SimilarityVector> SpecializeLhs(model::SimilarityVector const& lhs,
                                                         size_t col_match_index,
                                                         model::Similarity similarity) const;

    model::SimilarityVector GetSimilarityVector(size_t left_record, size_t right_record) const;

    std::pair<model::SimilarityVector, size_t> GetMaxRhsDecBounds(
            model::SimilarityVector const& lhs_sims) const;


};

}  // namespace algos::hymd
