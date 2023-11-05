#include "algorithms/md/hymd/lattice_traverser.h"

#include <cassert>

namespace algos::hymd {

bool LatticeTraverser::TraverseLattice(bool traverse_all) {
    SimilarityData& similarity_data = *similarity_data_;
    size_t const col_matches_num = similarity_data.GetColumnMatchNumber();
    model::SimilarityVector const& rhs_min_similarities = similarity_data.GetRhsMinSimilarities();
    model::FullLattice& lattice = *lattice_;
    model::MinPickerLattice& min_picker_lattice = *min_picker_lattice_;
    while (cur_level_ <= lattice.GetMaxLevel()) {
        std::vector<model::MdLatticeNodeInfo> level_mds = lattice.GetLevel(cur_level_);
        min_picker_lattice.PickMinimalMds(level_mds);
        std::vector<model::MdLatticeNodeInfo> cur = min_picker_lattice.GetAll();
        if (cur.empty()) {
            ++cur_level_;
            min_picker_lattice.Advance();
            continue;
        }
        std::vector<model::LatticeMd> mds_to_add_if_min;
        for (model::MdLatticeNodeInfo& node : cur) {
            model::SimilarityVector const& lhs_sims = node.lhs_sims;
            model::SimilarityVector& rhs_sims = *node.rhs_sims;
            model::SimilarityVector old_rhs_sims = rhs_sims;
            std::unordered_set<size_t> indices;
            for (size_t index = 0; index < col_matches_num; ++index) {
                if (old_rhs_sims[index] != 0.0) indices.insert(index);
            }
            for (size_t const index : indices) {
                rhs_sims[index] = 0.0;
            }
            model::SimilarityVector gen_max_rhs = lattice.GetMaxValidGeneralizationRhs(lhs_sims);
            auto [new_rhs_sims, is_unsupported] =
                    similarity_data.GetMaxRhsDecBounds(lhs_sims, recommendations_ptr_, min_support_,
                                                       old_rhs_sims, gen_max_rhs, indices);
            if (is_unsupported) {
                lattice.MarkUnsupported(lhs_sims);
                continue;
            }
            assert(new_rhs_sims.size() == col_matches_num);
            for (size_t const i : indices) {
                if (prune_nondisjoint_ && lhs_sims[i] != 0.0) continue;
                model::Similarity const new_rhs_sim = new_rhs_sims[i];
                if (new_rhs_sim > lhs_sims[i] && new_rhs_sim >= rhs_min_similarities[i] &&
                    new_rhs_sim > gen_max_rhs[i]) {
                    rhs_sims[i] = new_rhs_sim;
                }
            }
            for (size_t i = 0; i < col_matches_num; ++i) {
                std::optional<model::SimilarityVector> new_lhs_sims =
                        similarity_data.SpecializeLhs(lhs_sims, i);
                if (!new_lhs_sims.has_value()) continue;
                if (lattice.IsUnsupported(new_lhs_sims.value())) continue;
                for (size_t j = 0; j < col_matches_num; ++j) {
                    model::Similarity const new_lhs_sim = new_lhs_sims.value()[j];
                    model::Similarity const rhs_sim = old_rhs_sims[j];
                    if (prune_nondisjoint_ && new_lhs_sim != 0.0) continue;
                    if (rhs_sim > new_lhs_sim) {
                        mds_to_add_if_min.emplace_back(new_lhs_sims.value(), rhs_sim, j);
                    }
                }
            }
        }
        for (auto& md : mds_to_add_if_min) {
            lattice.AddIfMinAndNotUnsupported(md);
        }
        if (!traverse_all) return false;
        // TODO: if we specialized no LHSs, we can cut off the rest of the lattice here.
    }
    return true;
}

}  // namespace algos::hymd
