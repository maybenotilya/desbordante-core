#include "algorithms/md/hymd/lattice_traverser.h"

#include <cassert>

namespace algos::hymd {

bool LatticeTraverser::TraverseLattice(bool traverse_all, bool prune_nondisjoint) {
    SimilarityData& similarity_data = *similarity_data_;
    size_t const col_matches_num = similarity_data.GetColumnMatchNumber();
    model::SimilarityVector const& rhs_min_similarities = similarity_data.GetRhsMinSimilarities();
    model::FullLattice& lattice = *lattice_;
    model::MinPickerLattice& min_picker_lattice = *min_picker_lattice_;
    while (cur_level_ <= lattice.GetMaxLevel()) {
        std::vector<model::LatticeNodeSims> level_mds = lattice.GetLevel(cur_level_);
        min_picker_lattice.PickMinimalMds(level_mds);
        std::vector<model::LatticeNodeSims> cur = min_picker_lattice.GetAll();
        if (cur.empty()) {
            ++cur_level_;
            min_picker_lattice.Advance();
            if (!traverse_all) return false;
            continue;
        }
        std::vector<model::LatticeMd> mds_to_add;
        std::vector<model::LatticeMd> mds_to_add_if_min;
        for (model::LatticeNodeSims const& node : cur) {
            lattice.RemoveNode(node.lhs_sims);
            model::SimilarityVector const& lhs_sims = node.lhs_sims;
            model::SimilarityVector const& rhs_sims = node.rhs_sims;
            std::vector<double> gen_max_rhs = lattice.GetMaxValidGeneralizationRhs(lhs_sims);
            auto [new_rhs_sims, support] = similarity_data.GetMaxRhsDecBounds(
                    lhs_sims, recommendations_ptr_, min_support_, rhs_sims, prune_nondisjoint);
            if (support < min_support_) {
                lattice.MarkUnsupported(lhs_sims);
                continue;
            }
            assert(new_rhs_sims.size() == col_matches_num);
            for (size_t i = 0; i < new_rhs_sims.size(); ++i) {
                if (prune_nondisjoint && lhs_sims[i] != 0.0) continue;
                model::Similarity const new_rhs_sim = new_rhs_sims[i];
                if (new_rhs_sim > lhs_sims[i] && new_rhs_sim >= rhs_min_similarities[i] &&
                    new_rhs_sim > gen_max_rhs[i]) {
                    mds_to_add.emplace_back(lhs_sims, new_rhs_sim, i);
                }
            }
            for (size_t i = 0; i < col_matches_num; ++i) {
                std::optional<model::SimilarityVector> new_lhs_sims =
                        similarity_data.SpecializeLhs(node.lhs_sims, i);
                if (!new_lhs_sims.has_value()) continue;
                if (lattice.IsUnsupported(new_lhs_sims.value())) continue;
                for (size_t j = 0; j < col_matches_num; ++j) {
                    model::Similarity const new_lhs_sim = new_lhs_sims.value()[j];
                    model::Similarity const rhs_sim = rhs_sims[j];
                    if (prune_nondisjoint && new_lhs_sim != 0.0) continue;
                    if (rhs_sim > new_lhs_sim) {
                        mds_to_add_if_min.emplace_back(new_lhs_sims.value(), rhs_sim, j);
                    }
                }
            }
        }
        for (auto& md : mds_to_add) {
            lattice.Add(md);
        }
        for (auto& md : mds_to_add_if_min) {
            lattice.AddIfMinAndNotUnsupported(md);;
        }
    }
    return true;
}

}  // namespace algos::hymd
