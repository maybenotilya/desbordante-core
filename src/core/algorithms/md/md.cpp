#include "algorithms/md/md.h"

namespace model {

MD::MD(const RelationalSchema *left_schema, const RelationalSchema *right_schema,
       std::vector<ColumnMatch> column_matches, std::vector<LhsColumnSimilarityClassifier> lhs,
       model::ColumnSimilarityClassifier rhs)
    : left_schema_(left_schema),
      right_schema_(right_schema),
      column_matches_(std::move(column_matches)),
      lhs_(std::move(lhs)),
      rhs_(rhs) {}

std::string MD::ToString() const noexcept {
    std::stringstream ss;

    return ss.str();
}

}  // namespace model
