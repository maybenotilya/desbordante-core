#pragma once

#include <functional>
#include <unordered_set>

#include "algorithms/md/hymd/model/data_info.h"
#include "algorithms/md/hymd/model/similarity_metric/similarity_metric_calculator.h"
#include "config/exceptions.h"

namespace algos::hymd::model {

template <typename T, bool check = false>
class ImmediateMetricCalculator : SimilarityMetricCalculator {
    std::shared_ptr<ValueInfo<T> const> const value_info_left_;
    std::shared_ptr<ValueInfo<T> const> const value_info_right_;
    std::vector<PliCluster> const* const clusters_right_;
    std::function<double(T const&, T const&)> get_similarity_;
    double min_sim_;
    bool is_null_equal_null_;

private:
    model::Similarity GetSimilarity(ValueIdentifier value_id_left, ValueIdentifier value_id_right);

public:
    ImmediateMetricCalculator(std::shared_ptr<ValueInfo<T> const> value_info_left,
                              std::shared_ptr<ValueInfo<T> const> value_info_right,
                              std::vector<PliCluster> const* clusters_right,
                              std::function<double(T const&, T const&)> get_similarity,
                              double min_sim, bool is_null_equal_null)
        : value_info_left_(std::move(value_info_left)),
          value_info_right_(std::move(value_info_right)),
          clusters_right_(clusters_right),
          get_similarity_(std::move(get_similarity)),
          min_sim_(min_sim),
          is_null_equal_null_(is_null_equal_null) {}

    void MakeIndexes() final;
};

template <typename T, bool check>
model::Similarity ImmediateMetricCalculator<T, check>::GetSimilarity(
        ValueIdentifier value_id_left, ValueIdentifier value_id_right) {
    auto const& left_val_info = *value_info_left_;
    auto const& right_val_info = *value_info_right_;
    auto const& left_nulls = left_val_info.nulls;
    auto const& right_nulls = right_val_info.nulls;
    auto const& left_empty = left_val_info.empty;
    auto const& right_empty = right_val_info.empty;
    if (left_nulls.find(value_id_left) != left_nulls.end()) {
        if (is_null_equal_null_ && right_nulls.find(value_id_right) != right_nulls.end())
            return 1.0;
        return 0.0;
    }
    if (left_empty.find(value_id_left) != left_empty.end()) {
        if (right_empty.find(value_id_right) != right_empty.end()) return 1.0;
        return 0.0;
    }
    auto value_left = left_val_info.data[value_id_left];
    auto value_right = right_val_info.data[value_id_right];
    return get_similarity_(value_left, value_right);
}

template <typename T, bool check>
void ImmediateMetricCalculator<T, check>::MakeIndexes() {
    auto const& data_left_size = value_info_left_->data.size();
    auto const& data_right_size = value_info_right_->data.size();
    for (size_t value_id_left = 0; value_id_left < data_left_size; ++value_id_left) {
        std::vector<std::pair<double, RecordIdentifier>> sim_rec_id_vec;
        for (size_t value_id_right = 0; value_id_right < data_right_size; ++value_id_right) {
            model::Similarity similarity = GetSimilarity(value_id_left, value_id_right);
            if constexpr (check) {
                if (!(similarity >= 0 && similarity <= 1)) {
                    // Configuration error because bundled similarity functions
                    // are certain to be correct, but the same cannot be said
                    // about user-supplied functions
                    throw config::OutOfRange("Unexpected similarity (" +
                                             std::to_string(similarity) + ")");
                }
            }
            if (similarity < min_sim_) continue;
            decision_bounds_.push_back(similarity);
            similarity_matrix_[value_id_left][value_id_right] = similarity;
            for (RecordIdentifier record_id : clusters_right_->operator[](value_id_right)) {
                sim_rec_id_vec.emplace_back(similarity, record_id);
            }
        }
        if (sim_rec_id_vec.empty()) continue;
        std::sort(sim_rec_id_vec.begin(), sim_rec_id_vec.end(), std::greater<>{});
        std::vector<RecordIdentifier> records;
        records.reserve(sim_rec_id_vec.size());
        for (auto [_, rec] : sim_rec_id_vec) {
            records.push_back(rec);
        }
        SimInfo sim_info;
        double previous_similarity = sim_rec_id_vec.begin()->first;
        auto const it_begin = records.begin();
        for (size_t j = 0; j < sim_rec_id_vec.size(); ++j) {
            double const similarity = sim_rec_id_vec[j].first;
            if (similarity == previous_similarity) continue;
            auto const it_end = it_begin + static_cast<long>(j);
            std::sort(it_begin, it_end);
            sim_info[previous_similarity] = {it_begin, it_end};
            previous_similarity = similarity;
        }
        std::sort(records.begin(), records.end());
        sim_info[previous_similarity] = std::move(records);
        similarity_index_[value_id_left] = std::move(sim_info);
    }
    std::sort(decision_bounds_.begin(), decision_bounds_.end());
    decision_bounds_.erase(std::unique(decision_bounds_.begin(), decision_bounds_.end()),
                           decision_bounds_.end());
}

}  // namespace algos::hymd::model
