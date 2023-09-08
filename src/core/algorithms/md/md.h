#pragma once

#include "model/table/relational_schema.h"
#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/lhs_column_similarity_classifier.h"

namespace model {

// Based on the definition given in the article titled "Efficient Discovery of
// Matching Dependencies" by Philipp Schirmer, Thorsten Papenbrock, Ioannis
// Koumarelas, and Felix Naumann.
class MD {
private:
    RelationalSchema const *left_schema_;
    RelationalSchema const *right_schema_;
    std::vector<ColumnMatch> column_matches_;
    std::vector<LhsColumnSimilarityClassifier> lhs_;
    ColumnSimilarityClassifier rhs_;

public:
    MD(RelationalSchema const *left_schema, RelationalSchema const *right_schema,
       std::vector<ColumnMatch> column_matches, std::vector<LhsColumnSimilarityClassifier> lhs,
       ColumnSimilarityClassifier rhs);

    [[nodiscard]] std::string ToString() const noexcept;
};

}  // namespace model
