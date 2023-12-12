#include "algorithms/md/hymd/hymd.h"

#include <algorithm>

#include "algorithms/md/hymd/lattice/cardinality/min_picking_level_getter.h"
#include "model/index.h"
#include "model/table/column.h"

namespace {
[[maybe_unused]] void IntersectInPlace(std::set<model::Index>& set1,
                                       std::set<model::Index> const& set2) {
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
    similarity_data_.reset();
    lattice_.reset();
    recommendations_.clear();
    lattice_traverser_.reset();
    record_pair_inferrer_.reset();
}

void HyMD::LoadDataInternal() {
    left_schema_ = std::make_shared<RelationalSchema>(left_table_->GetRelationName());
    size_t const left_table_cols = left_table_->GetNumberOfColumns();
    for (model::Index i = 0; i < left_table_cols; ++i) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }
    if (right_table_ == nullptr) {
        right_schema_ = left_schema_;
        compressed_records_ = indexes::CompressedRecords::CreateFrom(*left_table_);
    } else {
        right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
        size_t const right_table_cols = right_table_->GetNumberOfColumns();
        for (model::Index i = 0; i < right_table_cols; ++i) {
            right_schema_->AppendColumn(right_table_->GetColumnName(i));
        }
        compressed_records_ = indexes::CompressedRecords::CreateFrom(*left_table_, *right_table_);
    }
    if (compressed_records_->GetLeftRecords().GetNumberOfRecords() == 0 ||
        compressed_records_->GetRightRecords().GetNumberOfRecords() == 0) {
        throw config::ConfigurationError("MD mining with either table empty is meaningless!");
    }
}

unsigned long long HyMD::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();
    // make column_match_col_indices_ here using column_matches_option_
    std::vector<std::pair<model::Index, model::Index>> column_match_col_indices;
    // make similarity measures here(?)
    std::vector<preprocessing::similarity_measure::SimilarityMeasure const*> sim_measures;
    for (auto const& [left_col_name, right_col_name, measure_ptr] : column_matches_option_) {
        column_match_col_indices.emplace_back(left_schema_->GetColumn(left_col_name)->GetIndex(),
                                              right_schema_->GetColumn(right_col_name)->GetIndex());
        sim_measures.push_back(measure_ptr.get());
    }
    similarity_data_ = SimilarityData::CreateFrom(
            compressed_records_.get(),
            std::move(column_match_col_indices), sim_measures, is_null_equal_null_);
    size_t const column_match_number = similarity_data_->GetColumnMatchNumber();
    assert(column_match_number != 0);
    lattice_ = std::make_unique<lattice::FullLattice>(column_match_number, [](...) { return 1; });
    lattice_traverser_ = std::make_unique<LatticeTraverser>(
            similarity_data_.get(), lattice_.get(), &recommendations_, min_support_,
            std::make_unique<lattice::cardinality::MinPickingLevelGetter>(lattice_.get()));
    record_pair_inferrer_ = std::make_unique<RecordPairInferrer>(similarity_data_.get(),
                                                                 lattice_.get(), &recommendations_);

    bool done = false;
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
    size_t const column_match_number = similarity_data_->GetColumnMatchNumber();
    for (size_t level = 0; level <= lattice_->GetMaxLevel(); ++level) {
        std::vector<lattice::MdLatticeNodeInfo> mds = lattice_->GetLevel(level);
        for (auto const& md : mds) {
            for (model::Index rhs_index = 0; rhs_index < column_match_number; ++rhs_index) {
                model::md::DecisionBoundary const rhs_sim = md.rhs_sims->operator[](rhs_index);
                if (rhs_sim == 0.0) continue;
                std::vector<model::md::ColumnMatch> column_matches;
                for (model::Index column_match_index = 0; column_match_index < column_match_number;
                     ++column_match_index) {
                    auto [left_col_index, right_col_index] =
                            similarity_data_->GetColMatchIndices(column_match_index);
                    column_matches.emplace_back(
                            left_col_index, right_col_index,
                            std::get<2>(column_matches_option_[column_match_index])->GetName());
                }
                std::vector<model::md::LhsColumnSimilarityClassifier> lhs;
                for (model::Index lhs_index = 0; lhs_index < md.lhs_sims.size(); ++lhs_index) {
                    model::md::DecisionBoundary const lhs_sim = md.lhs_sims[lhs_index];
                    lhs.emplace_back(similarity_data_->GetPreviousDecisionBound(lhs_sim, lhs_index),
                                     lhs_index, lhs_sim);
                }
                model::md::ColumnSimilarityClassifier rhs{rhs_index, rhs_sim};
                RegisterMd({left_schema_.get(), right_schema_.get(), column_matches, std::move(lhs),
                            rhs});
            }
        }
    }
}

}  // namespace algos::hymd
