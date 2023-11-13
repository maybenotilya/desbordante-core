#include "algorithms/md/hymd/lattice_traverser.h"

#include <cassert>

#include "model/index.h"

namespace algos::hymd {

bool LatticeTraverser::TraverseLattice(bool traverse_all) {
    SimilarityData& similarity_data = *similarity_data_;
    size_t const col_matches_num = similarity_data.GetColumnMatchNumber();
#ifndef NDEBUG
    std::vector<model::md::DecisionBoundary> const& rhs_min_similarities =
            similarity_data.GetRhsMinSimilarities();
#endif
    lattice::FullLattice& lattice = *lattice_;
    lattice::cardinality::MinPickerLattice& min_picker_lattice = *min_picker_lattice_;
    while (cur_level_ <= lattice.GetMaxLevel()) {
        std::vector<lattice::MdLatticeNodeInfo> level_mds = lattice.GetLevel(cur_level_);
        std::vector<lattice::ValidationInfo*> cur =
                min_picker_lattice.GetUncheckedLevelMds(level_mds);
        if (cur.empty()) {
            ++cur_level_;
            continue;
        }
        for (lattice::ValidationInfo* info : cur) {
            DecisionBoundaryVector& lhs_sims = info->info->lhs_sims;
            DecisionBoundaryVector& rhs_sims = *info->info->rhs_sims;
            std::unordered_set<model::Index>& indices = info->rhs_indices;
            DecisionBoundaryVector old_rhs_sims = rhs_sims;
            for (model::Index const index : indices) {
                rhs_sims[index] = 0.0;
            }
            std::vector<model::md::DecisionBoundary> gen_max_rhs =
                    lattice.GetMaxValidGeneralizationRhs(lhs_sims);
            auto [new_rhs_sims, is_unsupported] =
                    similarity_data.GetMaxRhsDecBounds(lhs_sims, recommendations_ptr_, min_support_,
                                                       old_rhs_sims, gen_max_rhs, indices);
            if (is_unsupported) {
                lattice.MarkUnsupported(lhs_sims);
                continue;
            }
            assert(new_rhs_sims.size() == col_matches_num);
            if (prune_nondisjoint_) {
                for (model::Index const i : indices) {
                    assert(lhs_sims[i] == 0.0);
                    model::md::DecisionBoundary const new_rhs_sim = new_rhs_sims[i];
                    if (new_rhs_sim == 0.0) continue;
                    assert(new_rhs_sim > gen_max_rhs[i]);
                    assert(new_rhs_sim > rhs_min_similarities[i]);
                    rhs_sims[i] = new_rhs_sim;
                }
            } else {
                for (model::Index const i : indices) {
                    model::md::DecisionBoundary const new_rhs_sim = new_rhs_sims[i];
                    if (new_rhs_sim == 0.0) continue;
                    assert(new_rhs_sim > gen_max_rhs[i]);
                    if (new_rhs_sim <= lhs_sims[i]) continue;
                    assert(new_rhs_sim > rhs_min_similarities[i]);
                    rhs_sims[i] = new_rhs_sim;
                }
            }
            if (prune_nondisjoint_) {
                for (model::Index i = 0; i < col_matches_num; ++i) {
                    model::md::DecisionBoundary& lhs_sim = lhs_sims[i];
                    std::optional<model::md::DecisionBoundary> new_sim =
                            similarity_data.SpecializeOneLhs(i, lhs_sim);
                    if (!new_sim.has_value()) continue;
                    model::md::DecisionBoundary const old_lhs_sim = lhs_sim;
                    lhs_sim = *new_sim;
                    if (lattice.IsUnsupported(lhs_sims)) {
                        lhs_sim = old_lhs_sim;
                        continue;
                    }
                    auto cur_lhs_spec_it = indices.find(i);
                    auto end_it = indices.end();
                    if (cur_lhs_spec_it == end_it) {
                        for (model::Index const j : indices) {
                            model::md::DecisionBoundary const rhs_sim = old_rhs_sims[j];
                            lattice.AddIfMinimal(lhs_sims, rhs_sim, j);
                        }
                    } else {
                        auto it = indices.begin();
                        for (; it != cur_lhs_spec_it; ++it) {
                            model::md::DecisionBoundary const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimal(lhs_sims, rhs_sim, *it);
                        }
                        for (++it; it != end_it; ++it) {
                            model::md::DecisionBoundary const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimal(lhs_sims, rhs_sim, *it);
                        }
                    }
                    lhs_sim = old_lhs_sim;
                }
            } else {
                for (model::Index i = 0; i < col_matches_num; ++i) {
                    model::md::DecisionBoundary& lhs_sim = lhs_sims[i];
                    std::optional<model::md::DecisionBoundary> new_sim =
                            similarity_data.SpecializeOneLhs(i, lhs_sim);
                    if (!new_sim.has_value()) continue;
                    model::md::DecisionBoundary const old_lhs_sim = lhs_sim;
                    lhs_sim = *new_sim;
                    if (lattice.IsUnsupported(lhs_sims)) {
                        lhs_sim = old_lhs_sim;
                        continue;
                    }
                    auto cur_lhs_spec_it = indices.find(i);
                    auto end_it = indices.end();
                    if (cur_lhs_spec_it == end_it) {
                        for (model::Index const j : indices) {
                            model::md::DecisionBoundary const rhs_sim = old_rhs_sims[j];
                            lattice.AddIfMinimal(lhs_sims, rhs_sim, j);
                        }
                    } else {
                        auto it = indices.begin();
                        for (; it != cur_lhs_spec_it; ++it) {
                            model::md::DecisionBoundary const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimal(lhs_sims, rhs_sim, *it);
                        }
                        model::md::DecisionBoundary const rhs_sim = old_rhs_sims[*it];
                        model::md::DecisionBoundary const new_lhs_sim = lhs_sims[*it];
                        if (rhs_sim > new_lhs_sim) {
                            lattice.AddIfMinimal(lhs_sims, rhs_sim, *it);
                        }
                        ++it;
                        for (; it != end_it; ++it) {
                            model::md::DecisionBoundary const rhs_sim = old_rhs_sims[*it];
                            lattice.AddIfMinimal(lhs_sims, rhs_sim, *it);
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
