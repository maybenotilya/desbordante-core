#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"

#include <cstddef>
#include <numeric>

#include <boost/asio.hpp>

#include "model/types/double_type.h"
#include "model/types/string_type.h"
#include "util/levenshtein_distance.h"

namespace {
struct SimTaskData {
    algos::hymd::indexes::SimilarityMatrixRow row;
    std::vector<model::md::DecisionBoundary> row_decision_bounds;
    algos::hymd::indexes::SimInfo sim_info;
    model::md::DecisionBoundary row_lowest = 1.0;
};

std::size_t GetLevenshteinBufferSize(auto const& right_string) {
    return right_string.size() + 1;
}

/* An optimized version of the Levenshtein distance computation algorithm from
 * https://en.wikipedia.org/wiki/Levenshtein_distance, using preallocated buffers
 */
unsigned LevenshteinDistance(auto const& l, auto const& r, unsigned* v0, unsigned* v1) {
    std::size_t const r_size = r.size();
    assert(v0 < v1);
    assert(GetLevenshteinBufferSize(r) == std::size_t(v1 - v0));
    std::size_t l_size = l.size();

    std::iota(v0, v0 + r_size + 1, 0);

    auto compute_arrays = [&](auto* v0, auto* v1, unsigned i) {
        *v1 = i + 1;
        auto const li = l[i];

        for (unsigned j = 0; j != r_size;) {
            unsigned const insert_cost = v1[j] + 1;
            unsigned const substition_cost = v0[j] + (li != r[j]);
            ++j;
            unsigned const deletion_cost = v0[j] + 1;

            v1[j] = std::min({deletion_cost, insert_cost, substition_cost});
        }
    };
    auto loop_to_l_size = [&l_size, &v0, &v1, &compute_arrays]() {
        for (unsigned i = 0; i != l_size; ++i) {
            compute_arrays(v0, v1, i);
            ++i;
            compute_arrays(v1, v0, i);
        }
    };
    if (l_size & 1) {
        --l_size;
        loop_to_l_size();
        compute_arrays(v0, v1, l_size);
        return v1[r_size];
    } else {
        loop_to_l_size();
        return v0[r_size];
    }
}
}  // namespace

namespace algos::hymd::preprocessing::similarity_measure {

indexes::ColumnSimilarityInfo LevenshteinSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const& clusters_right) const {
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
    auto process_value_id = [&](std::size_t const value_id_left) {
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
            if (!is_null_equal_null_) return;
            simple_case(task_info[value_id_left], right_nulls);
        } else if (left_empty.find(value_id_left) != left_empty.end()) {
            simple_case(task_info[value_id_left], right_empty);
        } else {
            auto const& string_left =
                    model::Type::GetValue<std::string>(data_info_left->GetAt(value_id_left));
            std::size_t const left_size = string_left.size();

            std::size_t const buf_size = GetLevenshteinBufferSize(string_left);
            auto buf = std::make_unique_for_overwrite<unsigned[]>(buf_size * 2);

            auto get_similarity = [&string_left, left_size, &data_info_right, buf1 = buf.get(),
                                   buf2 = buf.get() + buf_size](ValueIdentifier value_id_right) {
                auto const& right_nulls = data_info_right->GetNulls();
                if (right_nulls.find(value_id_right) != right_nulls.end()) return 0.0;
                auto const& right_empty = data_info_right->GetEmpty();
                if (right_empty.find(value_id_right) != right_empty.end()) return 0.0;

                auto const& string_right =
                        model::Type::GetValue<std::string>(data_info_right->GetAt(value_id_right));
                std::size_t const max_dist = std::max(left_size, string_right.size());
                // Left has to be second since that's what the function uses to determine the buffer
                // size it needs
                Similarity value = static_cast<Similarity>(
                                           max_dist - LevenshteinDistance(string_right, string_left,
                                                                          buf1, buf2)) /
                                   static_cast<Similarity>(max_dist);
                return value;
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
                for (RecordIdentifier record_id : clusters_right[value_id_right]) {
                    sim_rec_id_vec.emplace_back(similarity, record_id);
                }
            }
            if (sim_rec_id_vec.empty()) {
                assert(data.row.empty());
                assert(data.row_decision_bounds.empty());
                assert(data.row_lowest == 0.0);
                return;
            }
            std::sort(sim_rec_id_vec.begin(), sim_rec_id_vec.end(), std::greater<>{});
            Similarity previous_similarity = sim_rec_id_vec.begin()->first;
            auto fill_sim_info_set = [&data, &sim_rec_id_vec,
                                      &previous_similarity](model::Index to) {
                auto& prev_sim_set = data.sim_info[previous_similarity];
                prev_sim_set.reserve(to);
                for (model::Index j = 0; j < to; ++j) {
                    prev_sim_set.insert(sim_rec_id_vec[j].second);
                }
            };
            std::size_t const rec_num = sim_rec_id_vec.size();
            for (model::Index i = 0; i < rec_num; ++i) {
                Similarity const similarity = sim_rec_id_vec[i].first;
                if (similarity == previous_similarity) continue;
                fill_sim_info_set(i);
                previous_similarity = similarity;
            }
            fill_sim_info_set(rec_num);
        }
    };
    // TODO: add reusable thread pool
    boost::asio::thread_pool pool;
    for (ValueIdentifier left_value_id = 0; left_value_id < data_left_size; ++left_value_id) {
        boost::asio::post(
                pool, [left_value_id, &process_value_id]() { process_value_id(left_value_id); });
    }
    pool.join();
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

LevenshteinSimilarityMeasure::LevenshteinSimilarityMeasure(model::md::DecisionBoundary min_sim,
                                                           bool is_null_equal_null)
    : SimilarityMeasure("levenshtein_similarity", std::make_unique<model::StringType>(),
                        std::make_unique<model::DoubleType>()),
      is_null_equal_null_(is_null_equal_null),
      min_sim_(min_sim) {}

}  // namespace algos::hymd::preprocessing::similarity_measure
