#include "metric_verifier.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <numeric>
#include <stdexcept>

#include <easylogging++.h>

#include "convex_hull.h"
#include "options/common_options.h"
#include "options/names.h"
#include "options/descriptions.h"
#include "options/option_type.h"

namespace algos {

void TransformIndices(std::vector<unsigned int>& value) {
    if (value.empty()) {
        throw std::invalid_argument("Indices cannot be empty");
    }
    std::sort(value.begin(), value.end());
    value.erase(std::unique(value.begin(), value.end()), value.end());
}

decltype(MetricVerifier::ParameterOpt) MetricVerifier::ParameterOpt{
        {config::names::kParameter, config::descriptions::kDParameter}, [](auto value) {
            if (!(value >= 0 && value <= 1)) throw std::invalid_argument("Parameter out of range");
        }
};

decltype(MetricVerifier::LhsIndicesOpt) MetricVerifier::LhsIndicesOpt{
        {config::names::kLhsIndices, config::descriptions::kDLhsIndices}, TransformIndices
};

decltype(MetricVerifier::RhsIndicesOpt) MetricVerifier::RhsIndicesOpt{
        {config::names::kRhsIndices, config::descriptions::kDRhsIndices}, TransformIndices
};

decltype(MetricVerifier::MetricOpt) MetricVerifier::MetricOpt{
        {config::names::kMetric, config::descriptions::kDMetric}
};

decltype(MetricVerifier::AlgoOpt) MetricVerifier::AlgoOpt{
        {config::names::kMetricAlgorithm, config::descriptions::kDMetricAlgorithm},
        MetricAlgo::_values()[0]
};

decltype(MetricVerifier::QGramLengthOpt) MetricVerifier::QGramLengthOpt{
        {config::names::kQGramLength, config::descriptions::kDQGramLength}, 2
};

MetricVerifier::MetricVerifier() : Primitive({}) {
    RegisterOptions();
    MakeOptionsAvailable(config::GetOptionNames(config::EqualNulls));
}

void MetricVerifier::ValidateIndices(decltype(MetricVerifier::lhs_indices_)& value) const {
    size_t cols_count = relation_->GetSchema()->GetNumColumns();
    auto value_out_of_range = [cols_count](unsigned int i) { return i >= cols_count; };
    if (std::any_of(value.begin(), value.end(), value_out_of_range)) {
        throw std::runtime_error(
                "Column index should be less than the number of columns in the dataset.");
    }
}

void MetricVerifier::ValidateRhs(decltype(MetricVerifier::rhs_indices_)& value) {
    ValidateIndices(value);
    for (unsigned i : value) {
        model::TypedColumnData const& col = typed_relation_->GetColumnData(i);
        auto type_id = col.GetTypeId();
        if (type_id == +model::TypeId::kUndefined) {
            throw std::runtime_error(
                    "Column with index \"" + std::to_string(i) + "\" type undefined.");
        }
        if (type_id == +model::TypeId::kMixed) {
            throw std::runtime_error("Column with index \"" + std::to_string(i)
                                     + "\" contains values of different types.");
        }
        if (value.size() > 1 && !col.IsNumeric()) {
            throw std::runtime_error(
                    R"("euclidean" metric does not match RHS column type with index ")"
                    + std::to_string(i) + "\".");
        }
    }
}

void MetricVerifier::RegisterOptions() {
    auto check_lhs = [this](auto& value) { ValidateIndices(value); };
    auto check_rhs = [this](auto& value) { ValidateRhs(value); };
    auto true_func = [](...) { return true; };
    auto need_algo_and_q = [](Metric value) { return value == +Metric::cosine; };
    auto need_algo_only = [this](Metric value) {
        return value == +Metric::levenshtein
               || (value == +Metric::euclidean && rhs_indices_.size() == 1);
    };
    auto metric_check = [this](Metric value) {
        if (rhs_indices_.size() != 1 && value != +Metric::euclidean) {
            throw std::invalid_argument("Can't use this metric");
        }
    };
    auto algo_check = [this](MetricAlgo value) {
        const auto algo_unusable = "Can't use this algorithm with this metric and RHS indices.";
        if (value == +MetricAlgo::approx) {
            if (metric_ == +Metric::euclidean && rhs_indices_.size() == 1)
                throw std::invalid_argument(algo_unusable);
        }
        else if (value == +MetricAlgo::calipers) {
            if (!(metric_ == +Metric::euclidean && rhs_indices_.size() == 2))
                throw std::invalid_argument(algo_unusable);
        }
    };

    RegisterOption(config::EqualNulls.GetOption(&is_null_equal_null_));
    RegisterOption(config::NullDistInf.GetOption(&dist_to_null_infinity_));
    RegisterOption(ParameterOpt.GetOption(&parameter_));
    RegisterOption(LhsIndicesOpt.GetOption(&lhs_indices_).SetInstanceCheck(check_lhs));
    RegisterOption(RhsIndicesOpt.GetOption(&rhs_indices_).SetInstanceCheck(check_rhs)
                           .SetConditionalOpts(GetOptAvailFunc(),
                                               {{true_func, config::GetOptionNames(MetricOpt)}}));
    RegisterOption(MetricOpt.GetOption(&metric_).SetInstanceCheck(metric_check).SetConditionalOpts(
            GetOptAvailFunc(), {{need_algo_and_q, config::GetOptionNames(AlgoOpt, QGramLengthOpt)},
                                {need_algo_only, config::GetOptionNames(MetricOpt)}}));
    RegisterOption(AlgoOpt.GetOption(&algo_).SetInstanceCheck(algo_check));
    RegisterOption(QGramLengthOpt.GetOption(&q_));
}

void MetricVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable(config::GetOptionNames(config::NullDistInf, ParameterOpt, LhsIndicesOpt,
                                                RhsIndicesOpt, MetricOpt, AlgoOpt, QGramLengthOpt));
}

void MetricVerifier::FitInternal(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);
    data_stream.Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: metric FD verifying is meaningless.");
    }
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(data_stream,
                                                                       is_null_equal_null_);
}

unsigned long long MetricVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    metric_fd_holds_ = VerifyMetricFD();
    if (metric_fd_holds_) {
        LOG(INFO) << "Metric fd holds.";
    } else {
        LOG(INFO) << "Metric fd does not hold.";
    }

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

bool MetricVerifier::VerifyMetricFD() const {
    std::shared_ptr<util::PLI const>
        pli = relation_->GetColumnData(lhs_indices_[0]).GetPliOwnership();

    for (size_t i = 1; i < lhs_indices_.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(lhs_indices_[i]).GetPositionListIndex());
    }

    return std::all_of(pli->GetIndex().cbegin(), pli->GetIndex().cend(), GetCompareFunction());
}

MetricVerifier::CompareFunction MetricVerifier::GetCompareFunction() const {
    if (rhs_indices_.size() == 1) {
        model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
        switch (metric_) {
        case Metric::euclidean:
            if (!col.IsNumeric()) {
                throw std::runtime_error("\"euclidean\" metric does not match RHS column type.");
            }
            return [this](util::PLI::Cluster const& cluster) {
                return CompareNumericValues(cluster);
            };
        case Metric::levenshtein:
            if (col.GetTypeId() != +model::TypeId::kString) {
                throw std::runtime_error("\"levenshtein\" metric does not match RHS column type.");
            }
            return [this, &col](util::PLI::Cluster const& cluster) {
                auto const& type = dynamic_cast<model::StringType const&>(col.GetType());
                return CompareStringValues(
                    cluster, [&type](std::byte const* a, std::byte const* b) {
                        return type.Dist(a, b);
                    });
            };
        case Metric::cosine:
            if (col.GetTypeId() != +model::TypeId::kString) {
                throw std::runtime_error("\"cosine\" metric does not match RHS column type.");
            }
            return [this, &col](util::PLI::Cluster const& cluster) {
                auto const& type = dynamic_cast<model::StringType const&>(col.GetType());
                std::unordered_map<std::string, util::QGramVector> q_gram_map;
                return CompareStringValues(cluster, GetCosineDistFunction(type, q_gram_map));
            };
        }
    }
    if (algo_ == +MetricAlgo::calipers) {
        return [this](util::PLI::Cluster const& cluster) {
            return CalipersCompareNumericValues(cluster);
        };
    }
    return [this](util::PLI::Cluster const& cluster) {
        return CompareNumericMultiDimensionalValues(cluster);
    };
}

MetricVerifier::DistanceFunction MetricVerifier::GetCosineDistFunction(
    model::StringType const& type,
    std::unordered_map<std::string, util::QGramVector>& q_gram_map) const {
    return [this, &type, &q_gram_map](std::byte const* a, std::byte const* b) -> long double {
        std::string str1 = type.ValueToString(a);
        std::string str2 = type.ValueToString(b);
        if (str1.length() < q_ || str2.length() < q_) {
            throw std::runtime_error("q-gram length should not exceed the minimum string length "
                                     "in the dataset.");
        }
        util::QGramVector& v1 = q_gram_map.try_emplace(str1, str1, q_).first->second;
        util::QGramVector& v2 = q_gram_map.try_emplace(str2, str2, q_).first->second;
        return v1.CosineDistance(v2);
    };
}

bool MetricVerifier::CompareNumericValues(util::PLI::Cluster const& cluster) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    auto const& type = dynamic_cast<model::INumericType const&>(col.GetType());
    std::vector<std::byte const*> const& data = col.GetData();
    std::byte const* max_value = nullptr;
    std::byte const* min_value = nullptr;
    for (int row_index : cluster) {
        if (col.IsNull(row_index) || col.IsEmpty(row_index)) {
            if (dist_to_null_infinity_) return false;
            continue;
        }
        if (max_value == nullptr) {
            max_value = data[row_index];
            min_value = data[row_index];
            continue;
        }
        if (type.Compare(data[row_index], max_value) == model::CompareResult::kGreater) {
            max_value = data[row_index];
        } else if (type.Compare(data[row_index], min_value) == model::CompareResult::kLess) {
            min_value = data[row_index];
        }
        if (type.Dist(max_value, min_value) > parameter_)
            return false;
    }
    return true;
}

bool MetricVerifier::CompareStringValues(
    util::PLI::Cluster const& cluster, DistanceFunction const& dist_func) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[0]);
    std::vector<std::byte const*> const& data = col.GetData();
    for (size_t i = 0; i < cluster.size() - 1; ++i) {
        if (col.IsNull(cluster[i]) || col.IsEmpty(cluster[i])) {
            if (dist_to_null_infinity_) return false;
            continue;
        }
        long double max_dist = 0;
        for (size_t j = i + 1; j < cluster.size(); ++j) {
            if (col.IsNull(cluster[j]) || col.IsEmpty(cluster[j])) {
                if (dist_to_null_infinity_) return false;
                continue;
            }
            max_dist = std::max(max_dist, dist_func(data[cluster[i]], data[cluster[j]]));
            if (max_dist > parameter_) return false;
        }
        if (algo_ == +MetricAlgo::approx && max_dist * 2 < parameter_) return true;
    }
    return true;
}

static long double GetDistanceBetweenPoints(std::vector<long double> const& p1,
                                            std::vector<long double> const& p2) {
    assert(p1.size() == p2.size());
    assert(!p1.empty());
    return std::sqrt(std::inner_product(
        p1.cbegin(), p1.cend(), p2.cbegin(), 0.0, std::plus<>(),
        [](long double a, long double b) {
            return (a - b) * (a - b);
        }));
}

template <typename T>
std::vector<T> MetricVerifier::GetVectorOfPoints(
    util::PLI::Cluster const& cluster,
    std::function<void(T&, long double, size_t)> const& assignment_func) const {
    std::vector<T> points;
    for (int i : cluster) {
        bool has_values = false;
        bool has_nulls = false;
        T point;
        for (size_t j = 0; j < rhs_indices_.size(); ++j) {
            model::TypedColumnData const& col = typed_relation_->GetColumnData(rhs_indices_[j]);
            if (col.IsNull(i) || col.IsEmpty(i)) {
                if (dist_to_null_infinity_) {
                    return {};
                }
                if (has_values)
                    throw std::runtime_error("Some of the value coordinates are nulls.");
                has_nulls = true;
                continue;
            }
            if (has_nulls) throw std::runtime_error("Some of the value coordinates are nulls.");
            long double coord =
                col.GetType().GetTypeId() == +model::TypeId::kInt
                ? (long double)model::Type::GetValue<long long>(col.GetData()[i])
                : model::Type::GetValue<long double>(col.GetData()[i]);
            assignment_func(point, coord, j);
            has_values = true;
        }
        if (has_values) points.push_back(point);
    }
    return points;
}

bool MetricVerifier::CompareNumericMultiDimensionalValues(util::PLI::Cluster const& cluster) const {
    auto points = GetVectorOfPoints<std::vector<long double>>(
        cluster, [](auto& point, long double coord, [[maybe_unused]] size_t j) {
            point.push_back(coord);
        });
    if (points.empty()) return !dist_to_null_infinity_;
    for (size_t i = 0; i < points.size() - 1; ++i) {
        long double max_dist = 0;
        for (size_t j = i + 1; j < points.size(); ++j) {
            max_dist = std::max(max_dist, GetDistanceBetweenPoints(points[i], points[j]));
            if (max_dist > parameter_) return false;
        }
        if (algo_ == +MetricAlgo::approx && max_dist * 2 < parameter_) return true;
    }
    return true;
}

bool MetricVerifier::CalipersCompareNumericValues(const util::PLI::Cluster& cluster) const {
    auto points = GetVectorOfPoints<util::Point>(
        cluster, [](auto& point, long double coord, size_t j) {
            if (j == 0) point.x = coord;
            else point.y = coord;
        });
    if (points.empty()) return !dist_to_null_infinity_;
    auto pairs = util::GetAntipodalPairs(util::ConvexHull(points));
    return std::all_of(pairs.cbegin(), pairs.cend(), [this](auto const& pair) {
        return util::Point::EuclideanDistance(pair.first, pair.second) <= parameter_;
    });
}

}  // namespace algos
