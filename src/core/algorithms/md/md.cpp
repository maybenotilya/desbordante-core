#include "algorithms/md/md.h"

#include "model/table/column.h"

namespace model {

MD::MD(RelationalSchema const* left_schema, const RelationalSchema *right_schema,
       std::vector<ColumnMatch> column_matches, std::vector<LhsColumnSimilarityClassifier> lhs,
       model::ColumnSimilarityClassifier rhs)
    : left_schema_(left_schema),
      right_schema_(right_schema),
      column_matches_(std::move(column_matches)),
      lhs_(std::move(lhs)),
      rhs_(rhs) {}

std::string MD::ToString() const noexcept {
    std::stringstream ss;
    ss << "[";
    for (auto const& classifier : lhs_) {
        //if (classifier.GetDecisionBoundary() == 0.0) continue;
        auto const& column_match = column_matches_[classifier.GetColumnMatchIndex()];
        ss << " ";
        ss << column_match.similarity_function_name << "(" << left_schema_->GetName() << ":"
           << left_schema_->GetColumn(column_match.left_col_index)->GetName() << ", "
           << right_schema_->GetName() << ":"
           << right_schema_->GetColumn(column_match.right_col_index)->GetName()
           << ")>= " << classifier.GetDecisionBoundary();
        auto disp = classifier.GetMaxDisprovedBound();
        if (disp.has_value()) {
            ss << "(>" << disp.value() << ") ";
        }
        ss << "|";
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << "] -> ";
    auto const& classifier = rhs_;
    auto const& column_match = column_matches_[classifier.GetColumnMatchIndex()];
    ss << column_match.similarity_function_name << "(" << left_schema_->GetName() << ":"
       << left_schema_->GetColumn(column_match.left_col_index)->GetName() << ", "
       << right_schema_->GetName() << ":"
       << right_schema_->GetColumn(column_match.right_col_index)->GetName()
       << ")>=" << classifier.GetDecisionBoundary();
    return ss.str();
}

}  // namespace model
