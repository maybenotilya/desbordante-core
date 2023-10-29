#include "algorithms/md/hymd/model/similarity_measure/immediate_similarity_measure.h"

#include "config/exceptions.h"

namespace algos::hymd::model {

Similarity GetSimilarity(DataInfo const& data_info_left, DataInfo const& data_info_right,
                         SimilarityFunction const& compute_similarity, bool is_null_equal_null,
                         ValueIdentifier value_id_left, ValueIdentifier value_id_right) {
    auto const& left_data_info = data_info_left;
    auto const& right_data_info = data_info_right;
    auto const& left_nulls = left_data_info.GetNulls();
    auto const& right_nulls = right_data_info.GetNulls();
    auto const& left_empty = left_data_info.GetEmpty();
    auto const& right_empty = right_data_info.GetEmpty();
    if (left_nulls.find(value_id_left) != left_nulls.end()) {
        if (is_null_equal_null && right_nulls.find(value_id_right) != right_nulls.end())
            return 1.0;
        return 0.0;
    }
    if (left_empty.find(value_id_left) != left_empty.end()) {
        if (right_empty.find(value_id_right) != right_empty.end()) return 1.0;
        return 0.0;
    }
    std::byte const* value_left = left_data_info.GetAt(value_id_left);
    std::byte const* value_right = right_data_info.GetAt(value_id_right);
    return ::model::Type::GetValue<double>(compute_similarity(value_left, value_right).get());
}

std::tuple<DecisionBoundsVector, Similarity, SimilarityMatrix, SimilarityIndex>
ImmediateSimilarityMeasure::MakeIndexes(std::shared_ptr<DataInfo const> data_info_left,
                                        std::shared_ptr<DataInfo const> data_info_right,
                                        std::vector<PliCluster> const* clusters_right,
                                        double min_sim, bool is_null_equal_null) const {
    DecisionBoundsVector decision_bounds;
    SimilarityMatrix similarity_matrix;
    SimilarityIndex similarity_index;
    auto const& data_left_size = data_info_left->GetElementNumber();
    auto const& data_right_size = data_info_right->GetElementNumber();
    double lowest = 1.0;
    for (size_t value_id_left = 0; value_id_left < data_left_size; ++value_id_left) {
        std::vector<std::pair<Similarity, RecordIdentifier>> sim_rec_id_vec;
        for (size_t value_id_right = 0; value_id_right < data_right_size; ++value_id_right) {
            Similarity similarity =
                    GetSimilarity(*data_info_left, *data_info_right, compute_similarity_,
                                  is_null_equal_null, value_id_left, value_id_right);
            if (should_check_) {
                if (!(similarity >= 0 && similarity <= 1)) {
                    // Configuration error because bundled similarity functions
                    // are certain to be correct, but the same cannot be said
                    // about user-supplied functions
                    throw config::ConfigurationError("Unexpected similarity (" +
                                                     std::to_string(similarity) + ")");
                }
            }
            if (lowest > similarity) lowest = similarity;
            if (similarity < min_sim) continue;
            decision_bounds.push_back(similarity);
            similarity_matrix[value_id_left][value_id_right] = similarity;
            for (RecordIdentifier record_id : clusters_right->operator[](value_id_right)) {
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
        similarity_index[value_id_left] = std::move(sim_info);
    }
    std::sort(decision_bounds.begin(), decision_bounds.end());
    decision_bounds.erase(std::unique(decision_bounds.begin(), decision_bounds.end()),
                          decision_bounds.end());
    return {std::move(decision_bounds), lowest, std::move(similarity_matrix),
            std::move(similarity_index)};
}

}
