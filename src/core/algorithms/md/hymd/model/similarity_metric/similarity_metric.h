#pragma once

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>

#include "algorithms/md/hymd/types.h"
#include "algorithms/md/hymd/model/data_info.h"
#include "algorithms/md/hymd/model/dictionary_compressor/keyed_position_list_index.h"
#include "algorithms/md/hymd/model/similarity_metric/similarity_metric_calculator.h"
#include "model/types/builtin.h"
#include "model/types/null_type.h"
#include "model/types/numeric_type.h"
#include "model/types/type.h"

namespace algos::hymd::model {

class SimilarityMeasure {
private:
    std::string const name_;
    std::unique_ptr<::model::Type> const arg_type_;
    std::unique_ptr<::model::INumericType> const ret_type_;

protected:
    SimilarityFunction const compute_similarity_;

public:
    SimilarityMeasure(std::string name, std::unique_ptr<::model::Type> arg_type,
                     std::unique_ptr<::model::INumericType> ret_type,
                     SimilarityFunction compute_similarity)
        : name_(std::move(name)),
          arg_type_(std::move(arg_type)),
          ret_type_(std::move(ret_type)),
          compute_similarity_(std::move(compute_similarity)) {}

    virtual ~SimilarityMeasure() = default;

    [[nodiscard]] ::model::TypeId GetArgTypeId() const {
        return arg_type_->GetTypeId();
    }

    [[nodiscard]] ::model::Type const& GetArgType() const {
        return *arg_type_;
    }

    [[nodiscard]] std::string const& GetName() const {
        return name_;
    }

    [[nodiscard]] virtual std::tuple<DecisionBoundsVector, SimilarityMatrix, SimilarityIndex>
    MakeIndexes(std::shared_ptr<DataInfo const> data_info_left,
                std::shared_ptr<DataInfo const> data_info_right,
                std::vector<PliCluster> const* clusters_right, double min_sim,
                bool is_null_equal_null) const = 0;
};

}
