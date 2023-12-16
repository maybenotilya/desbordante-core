#pragma once

#include <memory>
#include <string>
#include <tuple>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/indexes/pli_cluster.h"
#include "algorithms/md/hymd/preprocessing/data_info.h"
#include "model/types/numeric_type.h"
#include "model/types/type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class SimilarityMeasure {
private:
    std::string const name_;
    std::unique_ptr<model::Type> const arg_type_;
    // Let R be a total order such that
    // \forall a in D \forall b in D (a R b -> sim_R(a) subseteq sim_R(b))
    // where sim is this similarity measure, R is defined on D x D
    // sim_R(c) := { (value1, value2) | sim(value1, value2) R c }
    // then we can use HyMD with minimal changes (enumerating the natural decision boundaries).
    // For the original algorithm D = [0.0, 1.0], R = >
    std::unique_ptr<model::INumericType> const ret_type_;

public:
    SimilarityMeasure(std::string name, std::unique_ptr<model::Type> arg_type,
                      std::unique_ptr<model::INumericType> ret_type)
        : name_(std::move(name)), arg_type_(std::move(arg_type)), ret_type_(std::move(ret_type)) {}

    virtual ~SimilarityMeasure() = default;

    [[nodiscard]] model::TypeId GetArgTypeId() const {
        return arg_type_->GetTypeId();
    }

    [[nodiscard]] model::Type const& GetArgType() const {
        return *arg_type_;
    }

    [[nodiscard]] std::string const& GetName() const {
        return name_;
    }

    [[nodiscard]] virtual indexes::ColumnSimilarityInfo MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right) const = 0;
};

}  // namespace algos::hymd::preprocessing::similarity_measure
