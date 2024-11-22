#pragma once

#include <type_traits>

#include "algorithms/algorithm.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/md_verifier/similarities/euclidean/euclidean.h"
#include "algorithms/md/md_verifier/similarities/levenshtein/levenshtein.h"
#include "algorithms/md/md_verifier/similarities/similarities.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "model/types/numeric_type.h"

namespace algos::md {

class MDVerifier : public Algorithm {
private:
    using DecisionBoundary = model::md::DecisionBoundary;

    config::InputTable input_table_;

    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    std::vector<DecisionBoundary> lhs_desicion_bondaries_;
    std::vector<DecisionBoundary> rhs_desicion_bondaries_;

    std::vector<std::shared_ptr<SimilarityMeasure>> lhs_similarity_measures_;
    std::vector<std::shared_ptr<SimilarityMeasure>> rhs_similarity_measures_;

    config::EqNullsType is_null_equal_null_;
    bool dist_from_null_is_infinity_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> relation_;

    std::unordered_set<std::pair<int, int>> pairs_violating_md_;

    bool md_holds_ = false;

    static void ValidateDecisionBoundaries(
            config::IndicesType const& indices,
            std::vector<DecisionBoundary> const& decision_boundaries);

    void ResetState() final;
    void RegisterOptions();
    void VerifyMD();

protected:
    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    MDVerifier();

    bool GetResult() const {
        return md_holds_;
    }
};

}  // namespace algos::md