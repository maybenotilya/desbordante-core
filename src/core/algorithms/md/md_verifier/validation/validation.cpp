#include "algorithms/md/md_verifier/validation/validation.h"

#include <boost/hof/first_of.hpp>

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/index_range.h"
#include "algorithms/md/md_verifier/validation/records_pairs.h"
#include "util/worker_thread_pool.h"

namespace {
using namespace algos::md;
using namespace algos;

std::vector<OneOfColumnMatchInfo> CreateColumnMatchesSimilarityInfos(
        hymd::SimilarityData const& similarity_data) {
    std::vector<hymd::ColumnMatchInfo> const& non_trivial_column_matches_info =
            similarity_data.GetColumnMatchesInfo();
    std::vector<std::pair<model::md::DecisionBoundary, model::Index>> const&
            trivial_column_matches_info = similarity_data.GetTrivialInfo();

    std::size_t total_column_matches_number =
            non_trivial_column_matches_info.size() + trivial_column_matches_info.size();
    std::vector<OneOfColumnMatchInfo> column_matches_similarity_infos(total_column_matches_number);
    std::vector<model::Index> const& sorted_to_original = similarity_data.GetIndexMapping();
    for (model::Index sorted_index : hymd::utility::IndexRange(sorted_to_original.size())) {
        model::Index original_index = sorted_to_original[sorted_index];

        column_matches_similarity_infos[original_index] =
                non_trivial_column_matches_info[sorted_index];
    }

    for (auto [decision_boundary, original_index] : trivial_column_matches_info) {
        column_matches_similarity_infos[original_index] = decision_boundary;
    }

    return column_matches_similarity_infos;
}

}  // namespace

namespace algos::md {
ColumnInfoView MDValidationCalculator::GetColumnInfo(
        hymd::ColumnMatchInfo const& column_match_info) {
    return {records_info_->GetLeftCompressor()
                    .GetPli(column_match_info.left_column_index)
                    .GetClusters(),
            records_info_->GetRightCompressor()
                    .GetPli(column_match_info.right_column_index)
                    .GetClusters(),
            column_match_info.similarity_info.similarity_matrix};
}

void MDValidationCalculator::ProcessUnmatchedPairs(hymd::ColumnMatchInfo const& column_match_info,
                                                   model::md::DecisionBoundary decision_boundary,
                                                   auto&& for_each_unmatched) {
    auto const& [left_clusters, right_clusters, similarity_matrix] =
            GetColumnInfo(column_match_info);
    for (hymd::ValueIdentifier left_value_id : hymd::utility::IndexRange(left_clusters.size())) {
        for (hymd::ValueIdentifier right_value_id :
             hymd::utility::IndexRange(right_clusters.size())) {
            model::md::Similarity similarity = 0.0;
            hymd::indexes::SimilarityMatrixRow const& similarity_matrix_row =
                    similarity_matrix[left_value_id];
            if (auto it = similarity_matrix_row.find(right_value_id);
                it != similarity_matrix_row.end()) {
                model::Index ccv_id = it->second;
                similarity = column_match_info.similarity_info.classifier_values[ccv_id];
            }
            if (similarity < decision_boundary) {
                for_each_unmatched(left_clusters[left_value_id], right_clusters[right_value_id],
                                   similarity);
            }
        }
    }
}

void MDValidationCalculator::RemoveNonMatchedLhsPairs(
        hymd::ColumnMatchInfo const& column_match_info,
        model::md::DecisionBoundary decision_boundary) {
    auto remove_unmatched = [this](hymd::indexes::PliCluster left_cluster,
                                   hymd::indexes::PliCluster right_cluster,
                                   [[maybe_unused]] model::md::Similarity similarity) {
        violating_records_.DeleteClusters(left_cluster, right_cluster);
    };

    ProcessUnmatchedPairs(column_match_info, decision_boundary, remove_unmatched);
}

void MDValidationCalculator::RemoveNonMatchedLhsPairsTrivial(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary) {
    if (similarity >= decision_boundary) {
        return;
    }

    violating_records_.Clear();
}

void MDValidationCalculator::InsertNonMatchedRhsPairsAndProcessViolations(
        hymd::ColumnMatchInfo const& column_match_info,
        model::md::DecisionBoundary decision_boundary) {
    auto add_unmatched = [this](hymd::indexes::PliCluster left_cluster,
                                hymd::indexes::PliCluster right_cluster,
                                model::md::Similarity similarity) {
        violating_records_.InsertClusters(left_cluster, right_cluster);
        InsertRhsSimilarities(left_cluster, right_cluster, similarity);
    };

    ProcessUnmatchedPairs(column_match_info, decision_boundary, add_unmatched);
}

void MDValidationCalculator::InsertNonMatchedRhsPairsAndProcessViolationsTrivial(
        model::md::Similarity similarity, model::md::DecisionBoundary decision_boundary) {
    if (similarity >= decision_boundary) {
        violating_records_.Clear();
        return;
    }

    // Need to do a lot of highlights...
    for (hymd::RecordIdentifier left_record :
         hymd::utility::IndexRange(records_info_->GetLeftCompressor().GetNumberOfRecords())) {
        for (hymd::RecordIdentifier right_record :
             hymd::utility::IndexRange(records_info_->GetRightCompressor().GetNumberOfRecords())) {
            rhs_records_pair_to_similarity_[{left_record, right_record}] = similarity;
            violating_records_.InsertPair(left_record, right_record);
        }
    }
}

void MDValidationCalculator::InsertRhsSimilarities(hymd::indexes::PliCluster const& left_cluster,
                                                   hymd::indexes::PliCluster const& right_cluster,
                                                   model::md::Similarity rhs_similarity) {
    for (hymd::RecordIdentifier left_record : left_cluster) {
        for (hymd::RecordIdentifier right_record : right_cluster) {
            auto it = violating_records_.GetPairs().find(left_record);
            if (it != violating_records_.GetPairs().end() &&
                it->second.find(right_record) != it->second.end()) {
                rhs_records_pair_to_similarity_[{left_record, right_record}] = rhs_similarity;
            }
        }
    }
}

void MDValidationCalculator::FindTrueRhsDecisionBoundary() {
    true_rhs_decision_boundary_ = rhs_column_similarity_classifier_.GetDecisionBoundary();
    for (auto& [left_record, records_set] : violating_records_.GetPairs()) {
        for (hymd::ValueIdentifier right_record : records_set) {
            true_rhs_decision_boundary_ =
                    std::min(true_rhs_decision_boundary_,
                             rhs_records_pair_to_similarity_.at({left_record, right_record}));
        }
    }
}

void MDValidationCalculator::FindRhsUnmatchedPairs(
        std::vector<OneOfColumnMatchInfo> column_matches_similarity_infos) {
    OneOfColumnMatchInfo const& rhs_column_match_info =
            column_matches_similarity_infos[rhs_column_similarity_classifier_
                                                    .GetColumnMatchIndex()];
    model::md::DecisionBoundary rhs_decision_boundary =
            rhs_column_similarity_classifier_.GetDecisionBoundary();

    std::visit(boost::hof::first_of(
                       [&](hymd::ColumnMatchInfo const& column_match_info) {
                           InsertNonMatchedRhsPairsAndProcessViolations(column_match_info,
                                                                        rhs_decision_boundary);
                       },
                       [&](TrivialColumnMatchInfo similarity) {
                           InsertNonMatchedRhsPairsAndProcessViolationsTrivial(
                                   similarity, rhs_decision_boundary);
                       }),
               rhs_column_match_info);
}

void MDValidationCalculator::FindAllLhsUnmatchedPairs(
        std::vector<OneOfColumnMatchInfo> column_matches_similarity_infos) {
    for (model::Index lhs_index = 0; lhs_index < lhs_column_similarity_classifiers_.size();
         ++lhs_index) {
        model::md::ColumnSimilarityClassifier const& clf =
                lhs_column_similarity_classifiers_[lhs_index];
        OneOfColumnMatchInfo const& column_match_info =
                column_matches_similarity_infos[clf.GetColumnMatchIndex()];
        std::visit(
                boost::hof::first_of(
                        [&](hymd::ColumnMatchInfo const& column_match_info) {
                            RemoveNonMatchedLhsPairs(column_match_info, clf.GetDecisionBoundary());
                        },
                        [&](TrivialColumnMatchInfo similarity) {
                            RemoveNonMatchedLhsPairsTrivial(similarity, clf.GetDecisionBoundary());
                        }),
                column_match_info);
    }
}

void MDValidationCalculator::ConstructResults() {
    holds_ = violating_records_.Empty();
    FindTrueRhsDecisionBoundary();
}

void MDValidationCalculator::Validate(util::WorkerThreadPool* thread_pool) {
    hymd::SimilarityData similarity_data =
            hymd::SimilarityData::CreateFrom(records_info_.get(), column_matches_, thread_pool)
                    .first;
    std::vector<OneOfColumnMatchInfo> column_matches_similarity_infos =
            CreateColumnMatchesSimilarityInfos(similarity_data);

    FindRhsUnmatchedPairs(column_matches_similarity_infos);
    FindAllLhsUnmatchedPairs(column_matches_similarity_infos);

    ConstructResults();
}
}  // namespace algos::md
