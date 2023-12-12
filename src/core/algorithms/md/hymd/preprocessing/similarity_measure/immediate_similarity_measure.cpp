#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"

#include <ranges>

#include "config/exceptions.h"
#include "util/parallel_for.h"

namespace algos::hymd::preprocessing::similarity_measure {

struct SimTaskData {
    indexes::SimilarityMatrixRow row;
    std::vector<model::md::DecisionBoundary> row_decision_bounds;
    indexes::SimInfo sim_info;
    model::md::DecisionBoundary row_lowest = 1.0;
};

indexes::ColumnSimilarityInfo ImmediateSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const* clusters_right, bool is_null_equal_null) const {
    std::vector<model::md::DecisionBoundary> decision_bounds;
    indexes::SimilarityMatrix similarity_matrix;
    indexes::SimilarityIndex similarity_index;
    std::size_t const data_left_size = data_info_left->GetElementNumber();
    Similarity lowest = 1.0;
    auto const& left_nulls = data_info_left->GetNulls();
    auto const& left_empty = data_info_left->GetEmpty();
    auto const& right_nulls = data_info_right->GetNulls();
    auto const& right_empty = data_info_right->GetEmpty();
    std::size_t const right_size = data_info_right->GetElementNumber();
    std::vector<SimTaskData> task_info(data_left_size);
    auto value_ids = std::ranges::views::iota(std::size_t(0), data_left_size);
    util::parallel_foreach(
            value_ids.begin(), value_ids.end(), 12, [&](std::size_t const value_id_left) {
                auto simple_case = [&](SimTaskData& data, auto const& collection) {
                    std::size_t const collection_size = collection.size();
                    assert(right_size != 0);
                    if (collection_size != right_size) {
                        data.row_lowest = 0.0;
                        if (collection.empty()) return;
                    }
                    data.row_decision_bounds.assign(collection_size, 1.0);
                    for (ValueIdentifier value_id : collection) {
                        data.row[value_id] = 1.0;
                    }
                    data.sim_info[1.0] = collection;
                };
                if (left_nulls.find(value_id_left) != left_nulls.end()) {
                    if (!is_null_equal_null) return;
                    simple_case(task_info[value_id_left], right_nulls);
                } else if (left_empty.find(value_id_left) != left_empty.end()) {
                    simple_case(task_info[value_id_left], right_empty);
                } else {
                    std::byte const* const value_left = data_info_left->GetAt(value_id_left);
                    auto get_similarity = [&](ValueIdentifier value_id_right) {
                        auto const& right_nulls = data_info_right->GetNulls();
                        if (right_nulls.find(value_id_right) != right_nulls.end()) return 0.0;
                        auto const& right_empty = data_info_right->GetEmpty();
                        if (right_empty.find(value_id_right) != right_empty.end()) return 0.0;

                        std::byte const* value_right = data_info_right->GetAt(value_id_right);
                        return CalculateSimilarity(value_left, value_right);
                    };
                    std::vector<std::pair<Similarity, RecordIdentifier>> sim_rec_id_vec;
                    std::size_t const data_right_size = data_info_right->GetElementNumber();
                    SimTaskData& data = task_info[value_id_left];
                    assert(data_right_size > 0);
                    for (ValueIdentifier value_id_right = 0; value_id_right < data_right_size;
                         ++value_id_right) {
                        Similarity similarity = get_similarity(value_id_right);
                        if (similarity < min_sim_) {
                            // Metanome keeps the actual value for some reason.
                            data.row_lowest = 0.0 /*similarity???*/;
                            continue;
                        }
                        if (data.row_lowest > similarity) data.row_lowest = similarity;
                        data.row_decision_bounds.push_back(similarity);
                        data.row[value_id_right] = similarity;
                        for (RecordIdentifier record_id :
                             clusters_right->operator[](value_id_right)) {
                            sim_rec_id_vec.emplace_back(similarity, record_id);
                        }
                    }
                    if (sim_rec_id_vec.empty()) {
                        assert(data.row.empty());
                        assert(data.row_decision_bounds.empty());
                        assert(data.row_lowest == 0.0);
                        return;
                    }
                    Similarity previous_similarity = sim_rec_id_vec.begin()->first;
                    auto fill_sim_info_set = [&data, &sim_rec_id_vec,
                                              &previous_similarity](model::Index to) {
                        auto& prev_sim_set = data.sim_info[previous_similarity];
                        prev_sim_set.reserve(to);
                        for (model::Index j = 0; j < to; ++j) {
                            prev_sim_set.insert(sim_rec_id_vec[j].second);
                        }
                    };
                    std::sort(sim_rec_id_vec.begin(), sim_rec_id_vec.end(), std::greater<>{});
                    std::size_t const rec_num = sim_rec_id_vec.size();
                    for (model::Index i = 0; i < rec_num; ++i) {
                        Similarity const similarity = sim_rec_id_vec[i].first;
                        if (similarity == previous_similarity) continue;
                        fill_sim_info_set(i);
                        previous_similarity = similarity;
                    }
                    fill_sim_info_set(rec_num);
                }
            });
    for (ValueIdentifier left_value_id = 0; left_value_id < data_left_size; ++left_value_id) {
        SimTaskData& task = task_info[left_value_id];
        if (task.row_decision_bounds.empty()) continue;
        similarity_index[left_value_id] = std::move(task.sim_info);
        similarity_matrix[left_value_id] = std::move(task.row);
        decision_bounds.insert(decision_bounds.end(), task.row_decision_bounds.begin(),
                               task.row_decision_bounds.end());
        if (task.row_lowest < lowest) lowest = task.row_lowest;
    }
    std::sort(decision_bounds.begin(), decision_bounds.end());
    decision_bounds.erase(std::unique(decision_bounds.begin(), decision_bounds.end()),
                          decision_bounds.end());
    return {std::move(decision_bounds), lowest, std::move(similarity_matrix),
            std::move(similarity_index)};
}

}  // namespace algos::hymd::preprocessing::similarity_measure
