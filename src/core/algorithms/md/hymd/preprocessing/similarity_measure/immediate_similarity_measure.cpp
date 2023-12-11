#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"

#include "config/exceptions.h"
#include "util/parallel_for.h"

namespace algos::hymd::preprocessing::similarity_measure {

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
        if (is_null_equal_null && right_nulls.find(value_id_right) != right_nulls.end()) return 1.0;
        return 0.0;
    }
    if (left_empty.find(value_id_left) != left_empty.end()) {
        if (right_empty.find(value_id_right) != right_empty.end()) return 1.0;
        return 0.0;
    }
    std::byte const* value_left = left_data_info.GetAt(value_id_left);
    std::byte const* value_right = right_data_info.GetAt(value_id_right);
    return model::Type::GetValue<model::md::DecisionBoundary>(
            compute_similarity(value_left, value_right).get());
}

struct SimCalcTask {
    std::shared_ptr<DataInfo const> const& data_info_left;
    std::shared_ptr<DataInfo const> const& data_info_right;
    std::vector<indexes::PliCluster> const* clusters_right;
    model::md::DecisionBoundary const min_sim;
    bool const is_null_equal_null;
    std::size_t const value_id_left;
    SimilarityFunction const& compute_similarity;
    indexes::SimilarityMatrixRow row{};
    std::vector<model::md::DecisionBoundary> row_decision_bounds{};
    indexes::SimInfo sim_info{};
    model::md::DecisionBoundary row_lowest = 1.0;

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
        return model::Type::GetValue<model::md::DecisionBoundary>(
                compute_similarity(value_left, value_right).get());
    }

    void MakeRow() {
        std::vector<std::pair<Similarity, RecordIdentifier>> sim_rec_id_vec;
        std::size_t const data_right_size = data_info_right->GetElementNumber();
        for (ValueIdentifier value_id_right = 0; value_id_right < data_right_size;
             ++value_id_right) {
            Similarity similarity =
                    GetSimilarity(*data_info_left, *data_info_right, compute_similarity,
                                  is_null_equal_null, value_id_left, value_id_right);
            if (similarity < min_sim) {
                // Metanome keeps the actual value for some reason.
                row_lowest = 0.0 /*similarity???*/;
                continue;
            }
            if (row_lowest > similarity) row_lowest = similarity;
            row_decision_bounds.push_back(similarity);
            row[value_id_right] = similarity;
            for (RecordIdentifier record_id : clusters_right->operator[](value_id_right)) {
                sim_rec_id_vec.emplace_back(similarity, record_id);
            }
        }
        if (sim_rec_id_vec.empty()) return;
        std::sort(sim_rec_id_vec.begin(), sim_rec_id_vec.end(), std::greater<>{});
        std::size_t const rec_num = sim_rec_id_vec.size();
        std::vector<RecordIdentifier> records;
        records.reserve(rec_num);
        for (auto [_, rec] : sim_rec_id_vec) {
            records.push_back(rec);
        }
        Similarity previous_similarity = sim_rec_id_vec.begin()->first;
        auto const it_begin = records.begin();
        for (model::Index j = 0; j < rec_num; ++j) {
            Similarity const similarity = sim_rec_id_vec[j].first;
            if (similarity == previous_similarity) continue;
            auto const it_end = it_begin + static_cast<long>(j);
            sim_info[previous_similarity] = {it_begin, it_end};
            previous_similarity = similarity;
        }
        sim_info[previous_similarity] = {it_begin, records.end()};
    }
};

indexes::ColumnSimilarityInfo ImmediateSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const* clusters_right, model::md::DecisionBoundary min_sim,
        bool is_null_equal_null) const {
    std::vector<model::md::DecisionBoundary> decision_bounds;
    indexes::SimilarityMatrix similarity_matrix;
    indexes::SimilarityIndex similarity_index;
    auto const& data_left_size = data_info_left->GetElementNumber();
    Similarity lowest = 1.0;
    std::vector<SimCalcTask> tasks;
    for (ValueIdentifier value_id_left = 0; value_id_left < data_left_size; ++value_id_left) {
        tasks.emplace_back(data_info_left, data_info_right, clusters_right, min_sim,
                           is_null_equal_null, value_id_left, compute_similarity_);
    }
    util::parallel_foreach(tasks.begin(), tasks.end(), 12, [](SimCalcTask& task) {
        task.MakeRow();
    });
    for (SimCalcTask& task : tasks) {
        if (task.row_decision_bounds.empty()) continue;
        similarity_index[task.value_id_left] = std::move(task.sim_info);
        similarity_matrix[task.value_id_left] = std::move(task.row);
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
