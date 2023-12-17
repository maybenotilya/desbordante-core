#pragma once

#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/lhs_column_similarity_classifier.h"
#include "model/table/relational_schema.h"

namespace model {

// Based on the definition given in the article titled "Efficient Discovery of
// Matching Dependencies" by Philipp Schirmer, Thorsten Papenbrock, Ioannis
// Koumarelas, and Felix Naumann.
class MD {
private:
    RelationalSchema const *left_schema_;
    RelationalSchema const *right_schema_;
    std::vector<md::ColumnMatch> column_matches_;
    std::vector<md::LhsColumnSimilarityClassifier> lhs_;
    md::ColumnSimilarityClassifier rhs_;

public:
    MD(RelationalSchema const *left_schema, RelationalSchema const *right_schema,
       std::vector<md::ColumnMatch> column_matches,
       std::vector<md::LhsColumnSimilarityClassifier> lhs, md::ColumnSimilarityClassifier rhs);

    [[nodiscard]] std::string ToStringFull() const noexcept;
    [[nodiscard]] std::string ToStringShort() const noexcept;
};

}  // namespace model
