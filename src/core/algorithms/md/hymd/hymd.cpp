#include "algorithms/md/hymd/hymd.h"

#include <algorithm>

#include "algorithms/md/hymd/model/dictionary_compressor/pli_intersector.h"
#include "util/intersect_sorted_sequences.h"

namespace {
[[maybe_unused]] void IntersectInPlace(std::set<size_t>& set1, std::set<size_t> const& set2) {
    auto it1 = set1.begin();
    auto it2 = set2.begin();
    while (it1 != set1.end() && it2 != set2.end()) {
        if (*it1 < *it2) {
            it1 = set1.erase(it1);
        } else if (*it2 < *it1) {
            ++it2;
        } else {
            ++it1;
            ++it2;
        }
    }
    set1.erase(it1, set1.end());
}
}  // namespace

namespace algos::hymd {

HyMD::HyMD() : MdAlgorithm({}) {}

void HyMD::ResetStateMd() {
    // make column_match_col_indices_ here using column_matches_option_
    std::vector<std::pair<size_t, size_t>> column_match_col_indices;
    // make similarity measures here(?)
    std::vector<model::SimilarityMeasure const*> sim_measures;
    for (auto const& [left_col_name, right_col_name, measure_ptr] : column_matches_option_) {
        column_match_col_indices.emplace_back(left_schema_->GetColumn(left_col_name)->GetIndex(),
                                              right_schema_->GetColumn(right_col_name)->GetIndex());
        sim_measures.push_back(measure_ptr.get());
    }
    similarity_data_ = SimilarityData::CreateFrom(
            compressed_records_.get(), std::move(rhs_min_similarities_),
            std::move(column_match_col_indices), sim_measures, is_null_equal_null_);
    lattice_ = std::make_unique<model::FullLattice>(similarity_data_->GetColumnMatchNumber());
    recommendations_.clear();
    lattice_traverser_ = std::make_unique<LatticeTraverser>(similarity_data_.get(), lattice_.get(),
                                                            &recommendations_, min_support_);
    record_pair_inferrer_ = std::make_unique<RecordPairInferrer>(similarity_data_.get(),
                                                                 lattice_.get(), &recommendations_);
}

void HyMD::LoadDataInternal() {
    left_schema_ = std::make_unique<RelationalSchema>(left_table_->GetRelationName());
    for (size_t i = 0; i < left_table_->GetNumberOfColumns(); ++i) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }
    right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
    for (size_t i = 0; i < right_table_->GetNumberOfColumns(); ++i) {
        right_schema_->AppendColumn(right_table_->GetColumnName(i));
    }

    compressed_records_ = model::CompressedRecords::CreateFrom(*left_table_, *right_table_);
}

unsigned long long HyMD::ExecuteInternal() {
    assert(similarity_data_->GetColumnMatchNumber() != 0);
    auto const start_time = std::chrono::system_clock::now();

    bool done;
    do {
        done = record_pair_inferrer_->InferFromRecordPairs();
        done = lattice_traverser_->TraverseLattice(done);
    } while (!done);

    RegisterResults();

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                 start_time)
            .count();
}

void HyMD::RegisterResults() {
    // Get the results from the lattice, sorted by LHS cardinality first (i.e.
    // number of nodes), then by similarity in each node, lexicographically:
    // [0.0 0.0] [0.1 0.0] [0.4 0.0] [1.0 0.0] [0.0 0.3] [0.0 0.6] [0.0 1.0]
    // [0.1 0.3] [0.1 0.6] [0.1 1.0] [0.4 0.3] [0.4 0.6] ...
    auto const& natural_decision_bounds_ = similarity_data_->GetNaturalDecisionBounds();
    size_t const column_match_number = similarity_data_->GetColumnMatchNumber();
    for (size_t level = 0; level <= lattice_->GetMaxLevel(); ++level) {
        std::vector<model::LatticeNodeSims> mds = lattice_->GetLevel(level);
        for (auto const& md : mds) {
            for (size_t i = 0; i < md.rhs_sims.size(); ++i) {
                double const rhs_sim = md.rhs_sims[i];
                if (rhs_sim == 0.0) continue;
                std::vector<::model::ColumnMatch> column_matches;
                for (size_t column_match_index = 0; column_match_index < column_match_number;
                     ++column_match_index) {
                    auto [left_col_index, right_col_index] =
                            similarity_data_->GetColMatchIndices(column_match_index);
                    column_matches.emplace_back(
                            left_col_index, right_col_index,
                            std::get<2>(column_matches_option_[column_match_index])->GetName());
                }
                std::vector<::model::LhsColumnSimilarityClassifier> lhs;
                for (size_t j = 0; j < md.lhs_sims.size(); ++j) {
                    double const lhs_sim = md.lhs_sims[j];
                    std::vector<double> bounds = natural_decision_bounds_[j];
                    auto it = std::lower_bound(bounds.begin(), bounds.end(), lhs_sim);
                    std::optional<double> max_disproved_bound;
                    if (it != bounds.begin()) max_disproved_bound = *--it;
                    lhs.emplace_back(max_disproved_bound, j, lhs_sim);
                }
                ::model::ColumnSimilarityClassifier rhs{i, rhs_sim};
                RegisterMd({left_schema_.get(), right_schema_.get(), column_matches, std::move(lhs),
                            rhs});
            }
        }
    }
}

}  // namespace algos::hymd
