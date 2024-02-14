#include "algorithms/md/hymd/hymd.h"

#include <algorithm>
#include <cstddef>

#include "algorithms/md/hymd/lattice/cardinality/min_picking_level_getter.h"
#include "algorithms/md/hymd/lattice_traverser.h"
#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/record_pair_inferrer.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/md_less.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "model/index.h"
#include "model/table/column.h"

namespace algos::hymd {

HyMD::HyMD() : MdAlgorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kLeftTable, kRightTable});
}

void HyMD::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kMinSupport, kPruneNonDisjoint, kColumnMatches});
}

void HyMD::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto min_support_default = [this]() {
        if (compressed_records_->OneTableGiven()) {
            return compressed_records_->GetLeftRecords().GetRecords().size() + 1;
        } else {
            return std::size_t(1);
        }
    };

    auto column_matches_default = [this]() {
        std::vector<std::tuple<std::string, std::string, std::shared_ptr<SimilarityMeasureCreator>>>
                column_matches_option;
        if (compressed_records_->OneTableGiven()) {
            std::size_t const num_columns = left_schema_->GetNumColumns();
            for (model::Index i = 0; i < num_columns; ++i) {
                std::string const column_name = left_schema_->GetColumn(i)->GetName();
                column_matches_option.emplace_back(
                        column_name, column_name,
                        std::make_shared<preprocessing::similarity_measure::
                                                 LevenshteinSimilarityMeasure::Creator>(0.7, true,
                                                                                        0));
            }
        } else {
            std::size_t const num_columns_left = left_schema_->GetNumColumns();
            std::size_t const num_columns_right = left_schema_->GetNumColumns();
            for (model::Index i = 0; i < num_columns_left; ++i) {
                std::string const column_name_left = left_schema_->GetColumn(i)->GetName();
                for (model::Index j = 0; j < num_columns_right; ++j) {
                    std::string const column_name_right = right_schema_->GetColumn(j)->GetName();
                    column_matches_option.emplace_back(
                            column_name_left, column_name_right,
                            std::make_shared<preprocessing::similarity_measure::
                                                     LevenshteinSimilarityMeasure::Creator>(
                                    0.7, true, 0));
                }
            }
        }
        return column_matches_option;
    };

    RegisterOption(Option{&left_table_, kLeftTable, kDLeftTable});
    RegisterOption(Option{&right_table_, kRightTable, kDRightTable, config::InputTable{nullptr}});

    RegisterOption(Option{&min_support_, kMinSupport, kDMinSupport, {min_support_default}});
    RegisterOption(Option{&prune_nondisjoint_, kPruneNonDisjoint, kDPruneNonDisjoint, true});
    RegisterOption(Option{
            &column_matches_option_, kColumnMatches, kDColumnMatches, {column_matches_default}});
}

void HyMD::ResetStateMd() {}

void HyMD::LoadDataInternal() {
    left_schema_ = std::make_shared<RelationalSchema>(left_table_->GetRelationName());
    std::size_t const left_table_cols = left_table_->GetNumberOfColumns();
    for (model::Index i = 0; i < left_table_cols; ++i) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }
    if (right_table_ == nullptr) {
        right_schema_ = left_schema_;
        compressed_records_ = indexes::CompressedRecords::CreateFrom(*left_table_);
    } else {
        right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
        std::size_t const right_table_cols = right_table_->GetNumberOfColumns();
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
    std::vector<std::tuple<std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure>,
                           model::Index, model::Index>>
            column_matches_info;
    for (auto const& [left_column_name, right_column_name, creator] : column_matches_option_) {
        column_matches_info.emplace_back(creator->MakeMeasure(),
                                         left_schema_->GetColumn(left_column_name)->GetIndex(),
                                         right_schema_->GetColumn(right_column_name)->GetIndex());
    }
    std::size_t const column_match_number = column_matches_info.size();
    assert(column_match_number != 0);
    // TODO: make infrastructure for depth level
    SimilarityData similarity_data =
            SimilarityData::CreateFrom(compressed_records_.get(), std::move(column_matches_info));
    lattice::FullLattice lattice{column_match_number, [](...) { return 1; }};
    Specializer specializer{similarity_data.GetColumnMatchesInfo(), &lattice, prune_nondisjoint_};
    LatticeTraverser lattice_traverser{
            &lattice,
            std::make_unique<lattice::cardinality::MinPickingLevelGetter>(&lattice),
            {compressed_records_.get(), similarity_data.GetColumnMatchesInfo(), min_support_,
             &lattice},
            &specializer};
    RecordPairInferrer record_pair_inferrer{&similarity_data, &lattice, &specializer};

    bool done = false;
    do {
        done = record_pair_inferrer.InferFromRecordPairs(lattice_traverser.TakeRecommendations());
        done = lattice_traverser.TraverseLattice(done);
    } while (!done);

    RegisterResults(similarity_data, lattice.GetAll());

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                 start_time)
            .count();
}

void HyMD::RegisterResults(SimilarityData const& similarity_data,
                           std::vector<lattice::MdLatticeNodeInfo> lattice_mds) {
    std::size_t const column_match_number = similarity_data.GetColumnMatchNumber();
    std::vector<model::md::ColumnMatch> column_matches;
    column_matches.reserve(column_match_number);
    for (model::Index column_match_index = 0; column_match_index < column_match_number;
         ++column_match_index) {
        auto [left_col_index, right_col_index] =
                similarity_data.GetColMatchIndices(column_match_index);
        column_matches.emplace_back(left_col_index, right_col_index,
                                    std::get<2>(column_matches_option_[column_match_index])
                                            ->GetSimilarityMeasureName());
    }
    std::vector<model::MD> mds;
    for (lattice::MdLatticeNodeInfo const& md : lattice_mds) {
        DecisionBoundaryVector& rhs_bounds = *md.rhs_bounds;
        for (model::Index rhs_index = 0; rhs_index < column_match_number; ++rhs_index) {
            model::md::DecisionBoundary const rhs_bound = rhs_bounds[rhs_index];
            if (rhs_bound == kLowestBound) continue;
            std::vector<model::md::LhsColumnSimilarityClassifier> lhs;
            for (model::Index lhs_index = 0; lhs_index < column_match_number; ++lhs_index) {
                model::md::DecisionBoundary const lhs_bound = md.lhs_bounds[lhs_index];
                lhs.emplace_back(similarity_data.GetPreviousDecisionBound(lhs_bound, lhs_index),
                                 lhs_index, lhs_bound);
            }
            model::md::ColumnSimilarityClassifier rhs{rhs_index, rhs_bound};
            mds.emplace_back(left_schema_.get(), right_schema_.get(), column_matches,
                             std::move(lhs), rhs);
        }
    }
    std::sort(mds.begin(), mds.end(), utility::MdLess);
    for (model::MD const& md : mds) {
        RegisterMd(md);
    }
}

}  // namespace algos::hymd
