#include "hymd.h"

namespace algos::hymd {

void HyMD::ResetStateMd() {
    sim_matrices_.clear();
    sim_indexes_.clear();
}

void HyMD::LoadDataInternal() {
    records_left_ = model::DictionaryCompressor::CreateFrom(*left_table_);
    records_right_ = model::DictionaryCompressor::CreateFrom(*right_table_);
}

unsigned long long HyMD::ExecuteInternal() {
    // ...
    FillSimilarities();

    bool done = false;
    do {
        done = InferFromRecordPairs();
        done = TraverseLattice(done);
    } while(!done);
    return 10031;
}

bool HyMD::TraverseLattice(bool traverse_all) {
    return false;
}

bool HyMD::InferFromRecordPairs() {
    return false;
}

void HyMD::FillSimilarities() {
    sim_matrices_.resize(column_matches_.size());
    sim_indexes_.resize(column_matches_.size());
    for (size_t i = 0; i < column_matches_.size(); ++i) {
        model::ColumnMatchInternal const& column_match = column_matches_[i];
        SimilarityMatrix& sim_matrix = sim_matrices_[i];
        SimilarityIndex& sim_index = sim_indexes_[i];
        std::unordered_map<std::string, size_t> const& left_mapping =
                records_left_.GetPlis()[column_match.left_col_index].GetMapping();
        std::unordered_map<std::string, size_t> const& right_mapping =
                records_right_.GetPlis()[column_match.right_col_index].GetMapping();
        sim_matrix.resize(left_mapping.size());
        sim_index.resize(left_mapping.size());
        for (auto const& [value_left, value_id_left] : left_mapping) {
            std::vector<std::pair<Similarity, ValueIdentifier>> sim_id_vec;
            for (auto const& [value_right, value_id_right] : right_mapping) {
                Similarity similarity = column_match.similarity_function_(value_left, value_right);
                sim_matrix[value_id_left][value_id_right] = similarity;
                sim_id_vec.emplace_back(similarity, value_id_right);
            }
            std::sort(sim_id_vec.begin(), sim_id_vec.end());
            auto& [sim_mapping, records] = sim_index[value_id_left];
            records.reserve(right_mapping.size());
            Similarity prev_sim = -1.0;
            for (size_t j = 0; j < sim_id_vec.size(); ++j) {
                auto [sim, rec] = sim_id_vec[j];
                records.emplace_back(rec);
                if (sim != prev_sim) {
                    sim_mapping[sim] = j;
                    prev_sim = sim;
                }
            }
        }
    }
}

}  // namespace algos
