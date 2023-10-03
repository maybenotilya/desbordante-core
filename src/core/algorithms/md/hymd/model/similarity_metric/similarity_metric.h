#pragma once

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>

#include "algorithms/md/hymd/model/data_info.h"
#include "algorithms/md/hymd/model/dictionary_compressor/keyed_position_list_index.h"
#include "algorithms/md/hymd/model/similarity_metric/similarity_metric_calculator.h"
#include "model/types/builtin.h"
#include "model/types/null_type.h"
#include "model/types/numeric_type.h"
#include "model/types/type.h"

namespace algos::hymd::model {

class SimilarityMetric {
public:
    using SimilarityFunction =
            std::function<std::unique_ptr<std::byte const>(std::byte const*, std::byte const*)>;

private:
    std::string const name_;
    std::unique_ptr<::model::Type> const arg_type_;
    std::unique_ptr<::model::INumericType> const ret_type_;
    SimilarityFunction const compute_similarity_;

public:
    SimilarityMetric(std::string name, std::unique_ptr<::model::Type> arg_type,
                     std::unique_ptr<::model::INumericType> ret_type,
                     SimilarityFunction compute_similarity)
        : name_(std::move(name)),
          arg_type_(std::move(arg_type)),
          ret_type_(std::move(ret_type)),
          compute_similarity_(std::move(compute_similarity)) {}
};

class AbstractSimilarityMetric {
    std::string name_;

protected:
    explicit AbstractSimilarityMetric(std::string name) : name_(std::move(name)) {}

public:
    [[nodiscard]] virtual std::type_index GetTypeIndex() const = 0;

    [[nodiscard]] virtual std::shared_ptr<void const> MakeValueInfo(
            KeyedPositionListIndex const& pli) const = 0;

    [[nodiscard]] std::string const& GetName() const {
        return name_;
    }

    [[nodiscard]] virtual std::shared_ptr<SimilarityMetricCalculator> MakeCalculator(
            std::shared_ptr<void const> value_info_left,
            std::shared_ptr<void const> value_info_right,
            std::vector<PliCluster> const* clusters_right, double min_sim,
            bool is_null_equal_null) const = 0;
};

template <typename T, typename RetType>
class SimilarityMetricOld : AbstractSimilarityMetric {
    std::function<RetType(T const&, T const&)> get_similarity_;

    [[nodiscard]] std::type_index GetTypeIndex() const final {
        return typeid(T);
    }

    [[nodiscard]] std::shared_ptr<const void> MakeValueInfo(
            KeyedPositionListIndex const& pli) const final {
        std::vector<T> data;
        std::unordered_set<ValueIdentifier> nulls;
        std::unordered_set<ValueIdentifier> empty;
        data.resize(pli.GetClusters().size());
        for (auto const& [string, value_id] : pli.GetMapping()) {
            if (string.empty()) {
                empty.insert(value_id);
                continue;
            }
            if (string == ::model::Null::kValue) {
                nulls.insert(value_id);
                continue;
            }
            data[value_id] = ::model::TypeConverter<T>::convert(string);
        }
        return std::make_shared<ValueInfo<T>>(std::move(data), std::move(nulls), std::move(empty));
    }

    [[nodiscard]] std::shared_ptr<SimilarityMetricCalculator> MakeCalculator(
            std::shared_ptr<void const> value_info_left,
            std::shared_ptr<void const> value_info_right,
            std::vector<PliCluster> const* clusters_right, double min_sim,
            bool is_null_equal_null) const final {
        return MakeCalculatorInternal(
                std::reinterpret_pointer_cast<ValueInfo<T> const>(value_info_left),
                std::reinterpret_pointer_cast<ValueInfo<T> const>(value_info_right), clusters_right,
                min_sim, is_null_equal_null);
    }

    [[nodiscard]] virtual std::shared_ptr<SimilarityMetricCalculator> MakeCalculatorInternal(
            std::shared_ptr<ValueInfo<T> const> value_info_left,
            std::shared_ptr<ValueInfo<T> const> value_info_right,
            std::vector<PliCluster> const* clusters_right, double min_sim,
            bool is_null_equal_null) const = 0;

protected:
    SimilarityMetricOld(std::string name, std::function<RetType(T const&, T const&)> get_similarity)
        : AbstractSimilarityMetric(std::move(name)), get_similarity_(std::move(get_similarity)) {}
};

}
