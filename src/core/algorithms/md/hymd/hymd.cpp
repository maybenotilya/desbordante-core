#include "hymd.h"

namespace algos::hymd {

void HyMD::ResetStateMd() {
    sim_matrices_.clear();
    sim_indexes_.clear();
    cur_level_ = 0;
    lattice_ = model::Lattice(column_matches_.size());
    cur_record_left_ = 0;
    cur_record_right_ = 0;
    recommendations_.clear();
    checked_recommendations_.clear();
}

void HyMD::LoadDataInternal() {
    left_schema_ = std::make_unique<RelationalSchema>(left_table_->GetRelationName());
    for (size_t i = 0; i < left_table_->GetNumberOfColumns(); ++i) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }
    right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
    for (size_t i = 0; i < right_table_->GetNumberOfColumns(); ++i) {
        right_schema_->AppendColumn(right_table_->GetColumnName(i));
    }

    records_left_ = model::DictionaryCompressor::CreateFrom(*left_table_);
    records_right_ = model::DictionaryCompressor::CreateFrom(*right_table_);
}

unsigned long long HyMD::ExecuteInternal() {
    // ...
    FillSimilarities();
    lattice_ = model::Lattice(column_matches_.size());

    bool done = false;
    do {
        done = InferFromRecordPairs();
        done = TraverseLattice(done);
    } while(!done);
    return 10031;
}

bool HyMD::TraverseLattice(bool traverse_all) {
    size_t const col_matches_num = column_matches_.size();
    while (cur_level_ < lattice_.GetMaxLevel()) {
        for (model::LatticeNode const& node : lattice_.GetLevel(cur_level_)) {
            lattice_.RemoveNode(node);
            model::SimilarityVector const& lhs_sims = node.lhs_sims;
            model::SimilarityVector const& rhs_sims = node.rhs_sims;
            std::vector<double> gen_max_rhs = lattice_.GetMaxValidGeneralizationRhs(lhs_sims);
            auto [new_rhs_sims, support] = GetMaxRhsDecBounds(lhs_sims);
            if (support < min_support_) {
                MarkUnsupported(lhs_sims);
                continue;
            }
            assert(new_rhs_sims.size() == column_matches_.size());
            for (size_t i = 0; i < new_rhs_sims.size(); ++i) {
                model::Similarity const new_rhs_sim = new_rhs_sims[i];
                if (new_rhs_sim > lhs_sims[i] && new_rhs_sim >= 0 && new_rhs_sim > gen_max_rhs[i]) {
                    lattice_.Add({lhs_sims, new_rhs_sim, i});
                }
            }
            for (size_t i = 0; i < col_matches_num; ++i) {
                if (!CanSpecializeLhs(lhs_sims, i)) continue;
                model::SimilarityVector new_lhs_sims = SpecializeLhs(node.lhs_sims, i);
                if (!IsSupported(new_lhs_sims)) continue;
                for (size_t j = 0; j < col_matches_num; ++j) {
                    model::Similarity const new_lhs_sim = new_lhs_sims[j];
                    model::Similarity const rhs_sim = rhs_sims[j];
                    if (rhs_sims[j] > new_lhs_sim) {
                        lattice_.AddIfMin({new_lhs_sims, rhs_sim, j});
                    }
                }
            }
        }
        ++cur_level_;
        if (!traverse_all) return false;
    }
    return true;
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
            std::vector<std::pair<model::Similarity, ValueIdentifier>> sim_id_vec;
            for (auto const& [value_right, value_id_right] : right_mapping) {
                model::Similarity similarity = column_match.similarity_function_(value_left, value_right);
                sim_matrix[value_id_left][value_id_right] = similarity;
                sim_id_vec.emplace_back(similarity, value_id_right);
            }
            std::sort(sim_id_vec.begin(), sim_id_vec.end());
            auto& [sim_mapping, records] = sim_index[value_id_left];
            records.reserve(right_mapping.size());
            model::Similarity prev_sim = -1.0;
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
