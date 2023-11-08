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
        std::vector<model::ValidationInfo*> cur =
                min_picker_lattice.GetUncheckedLevelMds(level_mds);
        if (cur.empty()) {
            ++cur_level_;
            continue;
        }
        for (model::ValidationInfo* info : cur) {
            model::SimilarityVector& lhs_sims = info->info->lhs_sims;
            model::SimilarityVector& rhs_sims = *info->info->rhs_sims;
            std::unordered_set<size_t>& indices = info->rhs_indices;
            model::SimilarityVector old_rhs_sims = rhs_sims;
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
            if (prune_nondisjoint_) {
                for (size_t const i : indices) {
                    assert(lhs_sims[i] == 0.0);
                    model::Similarity const new_rhs_sim = new_rhs_sims[i];
                    if (new_rhs_sim == 0.0) continue;
                    assert(new_rhs_sim > gen_max_rhs[i]);
                    rhs_sims[i] = new_rhs_sim;
                }
            } else {
                for (size_t const i : indices) {
                    model::Similarity const new_rhs_sim = new_rhs_sims[i];
                    if (new_rhs_sim == 0.0) continue;
                    assert(new_rhs_sim > gen_max_rhs[i]);
                    if (new_rhs_sim <= lhs_sims[i]) continue;
                    rhs_sims[i] = new_rhs_sim;
                }
            }
            if (prune_nondisjoint_) {
                for (size_t i = 0; i < col_matches_num; ++i) {
                    model::Similarity& lhs_sim = lhs_sims[i];
                    std::optional<model::Similarity> new_sim =
                            similarity_data.SpecializeOneLhs(i, lhs_sim);
                    if (!new_sim.has_value()) continue;
                    model::Similarity const old_lhs_sim = lhs_sim;
                    lhs_sim = *new_sim;
                    if (lattice.IsUnsupported(lhs_sims)) {
                        lhs_sim = old_lhs_sim;
                        continue;
                    }
                    auto cur_lhs_spec_it = indices.find(i);
                    auto end_it = indices.end();
                    if (cur_lhs_spec_it == end_it) {
                        for (size_t const j : indices) {
                            model::Similarity const rhs_sim = old_rhs_sims[j];
                            lattice.AddIfMinimalAndNotUnsupported(lhs_sims, rhs_sim, j);
                        }
                    } else {
                        auto it = indices.begin();
                        for (; it != cur_lhs_spec_it; ++it) {
                            model::Similarity const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimalAndNotUnsupported(lhs_sims, rhs_sim, *it);
                        }
                        for (++it; it != end_it; ++it) {
                            model::Similarity const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimalAndNotUnsupported(lhs_sims, rhs_sim, *it);
                        }
                    }
                    lhs_sim = old_lhs_sim;
                }
            } else {
                for (size_t i = 0; i < col_matches_num; ++i) {
                    model::Similarity& lhs_sim = lhs_sims[i];
                    std::optional<model::Similarity> new_sim =
                            similarity_data.SpecializeOneLhs(i, lhs_sim);
                    if (!new_sim.has_value()) continue;
                    model::Similarity const old_lhs_sim = lhs_sim;
                    lhs_sim = *new_sim;
                    if (lattice.IsUnsupported(lhs_sims)) {
                        lhs_sim = old_lhs_sim;
                        continue;
                    }
                    auto cur_lhs_spec_it = indices.find(i);
                    auto end_it = indices.end();
                    if (cur_lhs_spec_it == end_it) {
                        for (size_t const j : indices) {
                            model::Similarity const rhs_sim = old_rhs_sims[j];
                            lattice.AddIfMinimalAndNotUnsupported(lhs_sims, rhs_sim, j);
                        }
                    } else {
                        auto it = indices.begin();
                        for (; it != cur_lhs_spec_it; ++it) {
                            model::Similarity const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimalAndNotUnsupported(lhs_sims, rhs_sim, *it);
                        }
                        model::Similarity const rhs_sim = old_rhs_sims[*it];
                        model::Similarity const new_lhs_sim = lhs_sims[*it];
                        if (rhs_sim > new_lhs_sim) {
                            lattice.AddIfMinimalAndNotUnsupported(lhs_sims, rhs_sim, *it);
                        }
                        ++it;
                        for (; it != end_it; ++it) {
                            model::Similarity const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimalAndNotUnsupported(lhs_sims, rhs_sim, *it);
                        }
                    }
                    lhs_sim = old_lhs_sim;
                }
            }
        }
        if (!traverse_all) return false;
        // TODO: if we specialized no LHSs, we can cut off the rest of the lattice here.
    }
    return true;
}

}  // namespace algos::hymd
