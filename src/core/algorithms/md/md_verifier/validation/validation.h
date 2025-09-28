#pragma once

#include <variant>
#include <vector>

#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/md_verifier/cmptr.h"
#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/validation/records_pairs.h"
#include "algorithms/md/similarity.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::md {
struct ColumnInfoView {
    std::vector<hymd::indexes::PliCluster> const& left_clusters;
    std::vector<hymd::indexes::PliCluster> const& right_clusters;
    hymd::indexes::SimilarityMatrix const& similarity_matrix;
};

using TrivialColumnMatchInfo = model::md::DecisionBoundary;

using OneOfColumnMatchInfo = std::variant<hymd::ColumnMatchInfo, TrivialColumnMatchInfo>;

class MDValidationCalculator {
private:
    std::unique_ptr<hymd::indexes::RecordsInfo> records_info_;
    std::vector<OneOfColumnMatchInfo> column_matches_similarity_infos_;

    std::vector<CMPtr> column_matches_;
    std::vector<bool> non_informative_lhs_classifiers;  // Indicates whenever lhs classifier is non
                                                        // informative
    std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers_;
    model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier_;
    model::Index starting_lhs_classifier_index_ = 0;

    bool holds_ = true;
    bool validation_finished_ = false;

    model::md::DecisionBoundary true_rhs_decision_boundary_;
    std::shared_ptr<MDHighlights> highlights_;

    void FindAllNonMatchedPairs();

    model::Index GetStartingLhsClassifierIndex() {
        return starting_lhs_classifier_index_;
    }

    void ExecuteValidationFrom(hymd::ColumnMatchInfo column_match_info,
                               model::md::DecisionBoundary decision_boundary);
    void ExecuteValidationFromTrivial(model::md::Similarity similarity,
                                      model::md::DecisionBoundary decision_boundary);

    void ValidateAllLhsForRecordsPair(hymd::RecordIdentifier left_record_id,
                                      hymd::RecordIdentifier right_record_id);
    void ValidateRhsForRecordsPair(hymd::RecordIdentifier left_record_id,
                                   hymd::RecordIdentifier right_record_id);

    bool ValidateClassifierForPair(hymd::RecordIdentifier left_record_id,
                                   hymd::RecordIdentifier right_record_id,
                                   hymd::ColumnMatchInfo column_match_info,
                                   model::md::DecisionBoundary decision_boundary,
                                   auto&& on_lesser_boundary, auto&& on_greater_boundary);
    bool ValidateClassifierForPairTrivial(model::md::Similarity similarity,
                                          model::md::DecisionBoundary decision_boundary,
                                          auto&& on_lesser_boundary, auto&& on_greater_boundary);

    bool ValidateLhsClassifierForPair(hymd::RecordIdentifier left_record_id,
                                      hymd::RecordIdentifier right_record_id,
                                      hymd::ColumnMatchInfo column_match_info,
                                      model::md::DecisionBoundary decision_boundary);
    bool ValidateLhsClassifierForPairTrivial(model::md::Similarity similarity,
                                             model::md::DecisionBoundary decision_boundary);

    bool ValidateRhsClassifierForPair(hymd::RecordIdentifier left_record_id,
                                      hymd::RecordIdentifier right_record_id,
                                      hymd::ColumnMatchInfo column_match_info,
                                      model::md::DecisionBoundary decision_boundary);
    bool ValidateRhsClassifierForPairTrivial(model::md::Similarity similarity,
                                             model::md::DecisionBoundary decision_boundary);

    void CreateColumnMatchesSimilarityInfos(hymd::SimilarityData const& similarity_data);

    model::md::Similarity GetRecordsPairSimilarity(hymd::RecordIdentifier left_record_id,
                                                   hymd::RecordIdentifier right_record_id,
                                                   OneOfColumnMatchInfo column_match_info);

public:
    MDValidationCalculator(
            config::InputTable const& left_table, config::InputTable const& right_table,
            std::vector<CMPtr> const& column_matches,
            std::vector<model::md::ColumnSimilarityClassifier> const&
                    lhs_column_similarity_classifiers,
            model::md::ColumnSimilarityClassifier const& rhs_column_similarity_classifier,
            model::Index starting_lhs_classifier_index,
            std::shared_ptr<MDHighlights> const& highlights)
        : column_matches_(std::move(column_matches)),
          non_informative_lhs_classifiers(lhs_column_similarity_classifiers.size(), false),
          lhs_column_similarity_classifiers_(std::move(lhs_column_similarity_classifiers)),
          rhs_column_similarity_classifier_(std::move(rhs_column_similarity_classifier)),
          starting_lhs_classifier_index_(starting_lhs_classifier_index),
          true_rhs_decision_boundary_(rhs_column_similarity_classifier_.GetDecisionBoundary()),
          highlights_(highlights) {
        if (right_table == nullptr) {
            records_info_ = hymd::indexes::RecordsInfo::CreateFrom(*left_table);
        } else {
            records_info_ = hymd::indexes::RecordsInfo::CreateFrom(*left_table, *right_table);
        }
    }

    void Validate(util::WorkerThreadPool* thread_pool);

    bool Holds() const {
        return holds_;
    }

    model::md::DecisionBoundary GetTrueRhsDecisionBoundary() const {
        return true_rhs_decision_boundary_;
    }
};
}  // namespace algos::md
