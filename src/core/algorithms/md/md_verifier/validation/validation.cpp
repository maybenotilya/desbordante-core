#include "algorithms/md/md_verifier/validation/validation.h"

#include <ranges>

#include <boost/hof/first_of.hpp>

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/index_range.h"
#include "algorithms/md/md_verifier/validation/records_pairs.h"
#include "util/worker_thread_pool.h"

namespace algos::md {

void MDValidationCalculator::Validate(util::WorkerThreadPool* thread_pool) {
    hymd::SimilarityData similarity_data =
            hymd::SimilarityData::CreateFrom(records_info_.get(), column_matches_, thread_pool)
                    .first;

    CreateColumnMatchesSimilarityInfos(similarity_data);

    FindAllNonMatchedPairs();

    validation_finished_ = true;
}

void MDValidationCalculator::CreateColumnMatchesSimilarityInfos(
        hymd::SimilarityData const& similarity_data) {
    std::vector<hymd::ColumnMatchInfo> const& non_trivial_column_matches_info =
            similarity_data.GetColumnMatchesInfo();

    std::vector<std::pair<model::md::DecisionBoundary, model::Index>> const&
            trivial_column_matches_info = similarity_data.GetTrivialInfo();

    std::size_t total_column_matches_number =
            non_trivial_column_matches_info.size() + trivial_column_matches_info.size();
    column_matches_similarity_infos_.resize(total_column_matches_number);

    std::vector<model::Index> const& sorted_to_original = similarity_data.GetIndexMapping();
    for (model::Index sorted_index : hymd::utility::IndexRange(sorted_to_original.size())) {
        model::Index original_index = sorted_to_original[sorted_index];

        column_matches_similarity_infos_[original_index] =
                non_trivial_column_matches_info[sorted_index];
    }

    for (auto [decision_boundary, original_index] : trivial_column_matches_info) {
        column_matches_similarity_infos_[original_index] = decision_boundary;
    }
}

void MDValidationCalculator::FindAllNonMatchedPairs() {
    model::Index starting_classifier_index = GetStartingLhsClassifierIndex();

    OneOfColumnMatchInfo const& column_match_info =
            column_matches_similarity_infos_[starting_classifier_index];
    model::md::DecisionBoundary decision_boundary =
            lhs_column_similarity_classifiers_[starting_classifier_index].GetDecisionBoundary();

    std::visit(boost::hof::first_of(
                       [&](hymd::ColumnMatchInfo const& column_match_info) {
                           ExecuteValidationFrom(column_match_info, decision_boundary);
                       },
                       [&](model::md::Similarity similarity) {
                           ExecuteValidationFromTrivial(similarity, decision_boundary);
                       }),
               column_match_info);
}

void MDValidationCalculator::ExecuteValidationFrom(hymd::ColumnMatchInfo column_match_info,
                                                   model::md::DecisionBoundary decision_boundary) {
    hymd::indexes::SimilarityIndex const& similarity_index =
            column_match_info.similarity_info.similarity_index;

    auto it = std::ranges::lower_bound(column_match_info.similarity_info.classifier_values,
                                       decision_boundary);
    if (it == column_match_info.similarity_info.classifier_values.end()) {
        // No similarity >= decision boundary, lhs fails
        return;
    }
    hymd::ColumnClassifierValueId lower_bound_ccv_id =
            std::distance(column_match_info.similarity_info.classifier_values.begin(), it);

    for (hymd::ValueIdentifier left_value_id : hymd::utility::IndexRange(similarity_index.size())) {
        hymd::indexes::PliCluster const& left_pli_cluster =
                records_info_->GetLeftCompressor()
                        .GetPli(column_match_info.left_column_index)
                        .GetClusters()[left_value_id];
        hymd::indexes::RecSet const* upper_set =
                similarity_index[left_value_id].GetUpperSet(lower_bound_ccv_id);
        if (upper_set == nullptr) {
            continue;
        }
        for (hymd::RecordIdentifier left_record_id : left_pli_cluster) {
            for (hymd::RecordIdentifier right_record_id : *upper_set) {
                if (validation_finished_) {
                    return;
                }
                ValidateAllLhsForRecordsPair(left_record_id, right_record_id);
            }
        }
    }
}

void MDValidationCalculator::ExecuteValidationFromTrivial(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary) {
    if (similarity < decision_boundary) {
        return;
    }

    // Validate for all pairs if dependency holds
    // Not recommended to start from trivial column match
    for (hymd::RecordIdentifier left_record_id :
         hymd::utility::IndexRange(records_info_->GetLeftCompressor().GetNumberOfRecords())) {
        for (hymd::RecordIdentifier right_record_id :
             hymd::utility::IndexRange(records_info_->GetRightCompressor().GetNumberOfRecords())) {
            if (validation_finished_) {
                return;
            }
            ValidateAllLhsForRecordsPair(left_record_id, right_record_id);
        }
    }
}

void MDValidationCalculator::ValidateAllLhsForRecordsPair(hymd::RecordIdentifier left_record_id,
                                                          hymd::RecordIdentifier right_record_id) {
    auto validate_fn = [&](model::Index lhs_classifier_index) {
        model::md::DecisionBoundary decision_boundary =
                lhs_column_similarity_classifiers_[lhs_classifier_index].GetDecisionBoundary();
        OneOfColumnMatchInfo const& column_match_info =
                column_matches_similarity_infos_[lhs_classifier_index];
        return std::visit(boost::hof::first_of(
                                  [&](hymd::ColumnMatchInfo const& column_match_info) {
                                      return ValidateLhsClassifierForPair(
                                              left_record_id, right_record_id, column_match_info,
                                              decision_boundary);
                                  },
                                  [&](model::md::Similarity similarity) {
                                      return ValidateLhsClassifierForPairTrivial(similarity,
                                                                                 decision_boundary);
                                  }),
                          column_match_info);
    };

    bool all_lhs_holds = std::ranges::all_of(
            hymd::utility::IndexRange(lhs_column_similarity_classifiers_.size()),
            [&](model::Index lhs_classifier_index) {
                if (lhs_classifier_index == GetStartingLhsClassifierIndex()) {
                    return true;
                }
                return validate_fn(lhs_classifier_index);
            });

    if (!all_lhs_holds) {
        return;
    }

    ValidateRhsForRecordsPair(left_record_id, right_record_id);
}

bool MDValidationCalculator::ValidateClassifierForPair(
        hymd::RecordIdentifier left_record_id, hymd::RecordIdentifier right_record_id,
        hymd::ColumnMatchInfo column_match_info, model::md::DecisionBoundary decision_boundary,
        auto&& on_lesser_boundary, auto&& on_greater_boundary) {
    auto it = std::ranges::lower_bound(column_match_info.similarity_info.classifier_values,
                                       decision_boundary);
    if (it == column_match_info.similarity_info.classifier_values.end()) {
        return on_greater_boundary();
    }
    hymd::ColumnClassifierValueId lower_bound_ccv_id =
            std::distance(column_match_info.similarity_info.classifier_values.begin(), it);

    if (lower_bound_ccv_id == 0) {
        return on_lesser_boundary();
    }

    hymd::ValueIdentifier left_value_id =
            records_info_->GetLeftCompressor()
                    .GetRecords()[left_record_id][column_match_info.left_column_index];
    hymd::ValueIdentifier right_value_id =
            records_info_->GetRightCompressor()
                    .GetRecords()[right_record_id][column_match_info.right_column_index];

    hymd::indexes::SimilarityMatrixRow const& sim_matrix_row =
            column_match_info.similarity_info.similarity_matrix[left_value_id];

    if (auto it = sim_matrix_row.find(right_value_id); it != sim_matrix_row.end()) {
        hymd::ColumnClassifierValueId ccv_id = (*it).second;
        if (ccv_id >= lower_bound_ccv_id) {
            return true;
        }
    }

    return false;
}

bool MDValidationCalculator::ValidateClassifierForPairTrivial(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary,
        auto&& on_lesser_boundary, auto&& on_greater_boundary) {
    if (similarity >= decision_boundary) {
        return on_lesser_boundary();
    }
    return on_greater_boundary();
}

bool MDValidationCalculator::ValidateLhsClassifierForPair(
        hymd::RecordIdentifier left_record_id, hymd::RecordIdentifier right_record_id,
        hymd::ColumnMatchInfo column_match_info, model::md::DecisionBoundary decision_boundary) {
    auto on_lesser_boundary = [&]() {
        // Mark as non informative
        return true;
    };

    auto on_greater_boundary = [&]() {
        validation_finished_ = true;
        holds_ = true;
        return false;
    };

    return ValidateClassifierForPair(left_record_id, right_record_id, column_match_info,
                                     decision_boundary, on_lesser_boundary, on_greater_boundary);
}

bool MDValidationCalculator::ValidateLhsClassifierForPairTrivial(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary) {
    auto on_lesser_boundary = [&]() {
        // Mark as non informative
        return true;
    };

    auto on_greater_boundary = [&]() {
        validation_finished_ = true;
        holds_ = true;
        return false;
    };

    return ValidateClassifierForPairTrivial(similarity, decision_boundary, on_lesser_boundary,
                                            on_greater_boundary);
}

bool MDValidationCalculator::ValidateRhsClassifierForPair(
        hymd::RecordIdentifier left_record_id, hymd::RecordIdentifier right_record_id,
        hymd::ColumnMatchInfo column_match_info, model::md::DecisionBoundary decision_boundary) {
    auto on_lesser_boundary = [&]() {
        validation_finished_ = true;
        holds_ = true;
        return true;
    };

    auto on_greater_boundary = [&]() { return false; };

    return ValidateClassifierForPair(left_record_id, right_record_id, column_match_info,
                                     decision_boundary, on_lesser_boundary, on_greater_boundary);
}

bool MDValidationCalculator::ValidateRhsClassifierForPairTrivial(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary) {
    auto on_lesser_boundary = [&]() {
        validation_finished_ = true;
        holds_ = true;
        return true;
    };

    auto on_greater_boundary = [&]() { return false; };

    return ValidateClassifierForPairTrivial(similarity, decision_boundary, on_lesser_boundary,
                                            on_greater_boundary);
}

void MDValidationCalculator::ValidateRhsForRecordsPair(hymd::RecordIdentifier left_record_id,
                                                       hymd::RecordIdentifier right_record_id) {
    model::md::DecisionBoundary decision_boundary =
            rhs_column_similarity_classifier_.GetDecisionBoundary();
    OneOfColumnMatchInfo const& column_match_info = column_matches_similarity_infos_.back();

    bool rhs_holds = std::visit(
            boost::hof::first_of(
                    [&](hymd::ColumnMatchInfo const& column_match_info) {
                        return ValidateRhsClassifierForPair(left_record_id, right_record_id,
                                                            column_match_info, decision_boundary);
                    },
                    [&](model::md::Similarity similarity) {
                        return ValidateRhsClassifierForPairTrivial(similarity, decision_boundary);
                    }),
            column_match_info);

    if (!rhs_holds) {
        holds_ = false;

        model::md::Similarity similarity =
                GetRecordsPairSimilarity(left_record_id, right_record_id, column_match_info);
        highlights_->AddHighlight(left_record_id, right_record_id, similarity);
        true_rhs_decision_boundary_ = std::min(true_rhs_decision_boundary_, similarity);
    }
}

model::md::Similarity MDValidationCalculator::GetRecordsPairSimilarity(
        hymd::RecordIdentifier left_record_id, hymd::RecordIdentifier right_record_id,
        OneOfColumnMatchInfo column_match_info) {
    return std::visit(
            boost::hof::first_of(
                    [&](hymd::ColumnMatchInfo const& column_match_info) {
                        hymd::ValueIdentifier left_value_id =
                                records_info_->GetLeftCompressor()
                                        .GetRecords()[left_record_id]
                                                     [column_match_info.left_column_index];
                        hymd::ValueIdentifier right_value_id =
                                records_info_->GetRightCompressor()
                                        .GetRecords()[right_record_id]
                                                     [column_match_info.right_column_index];

                        hymd::indexes::SimilarityMatrixRow const& sim_matrix_row =
                                column_match_info.similarity_info.similarity_matrix[left_value_id];

                        model::md::Similarity similarity = 0.0;
                        if (auto it = sim_matrix_row.find(right_value_id);
                            it != sim_matrix_row.end()) {
                            hymd::ColumnClassifierValueId ccv_id = (*it).second;
                            similarity =
                                    column_match_info.similarity_info.classifier_values[ccv_id];
                        }

                        return similarity;
                    },
                    [&](model::md::Similarity similarity) { return similarity; }),
            column_match_info);
}

}  // namespace algos::md
