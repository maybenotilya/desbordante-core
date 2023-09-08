#pragma once

#include <cstddef>

namespace model {

class ColumnSimilarityClassifier {
private:
    size_t column_match_index_;
    double decision_boundary_;

public:
    ColumnSimilarityClassifier(size_t column_match_index, double decision_boundary)
        : column_match_index_(column_match_index), decision_boundary_(decision_boundary) {}

    [[nodiscard]] size_t GetColumnMatchIndex() const noexcept {
        return column_match_index_;
    }

    [[nodiscard]] double GetDecisionBoundary() const noexcept {
        return decision_boundary_;
    }
};

}  // namespace model