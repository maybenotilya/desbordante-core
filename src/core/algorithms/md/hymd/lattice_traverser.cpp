#include "algorithms/md/hymd/lattice_traverser.h"

#include <cassert>

#include "model/index.h"

namespace algos::hymd {

class SetForScope {
    model::md::DecisionBoundary& ref_;
    model::md::DecisionBoundary const old_value_;

public:
    SetForScope(model::md::DecisionBoundary& ref, model::md::DecisionBoundary const new_value)
        : ref_(ref), old_value_(ref) {
        ref_ = new_value;
    }

    ~SetForScope() {
        ref_ = old_value_;
    }
};

bool LatticeTraverser::TraverseLattice(bool traverse_all) {
    SimilarityData& similarity_data = *similarity_data_;
    size_t const col_matches_num = similarity_data.GetColumnMatchNumber();
    lattice::FullLattice& lattice = *lattice_;
    lattice::LevelGetter& level_getter = *level_getter_;
    while (level_getter.AreLevelsLeft()) {
        std::vector<lattice::ValidationInfo> mds = level_getter.GetCurrentMds();
        if (mds.empty()) {
            continue;
        }

        for (lattice::ValidationInfo& info : mds) {
            auto [violations, to_specialize, is_unsupported] =
                    similarity_data.Validate(lattice, info, min_support_);
            DecisionBoundaryVector& lhs_sims = info.info->lhs_sims;
            for (auto const& rhs_violations : violations) {
                recommendations_ptr_->insert(rhs_violations.begin(), rhs_violations.end());
            }
            // The above loop can be done concurrently with either of the below.
            if (is_unsupported) {
                lattice.MarkUnsupported(lhs_sims);
                continue;
            }
            if (prune_nondisjoint_) {
                for (model::Index lhs_spec_index = 0; lhs_spec_index < col_matches_num;
                     ++lhs_spec_index) {
                    model::md::DecisionBoundary& lhs_sim = lhs_sims[lhs_spec_index];
                    std::optional<model::md::DecisionBoundary> const new_sim =
                            similarity_data.SpecializeOneLhs(lhs_spec_index, lhs_sim);
                    if (!new_sim.has_value()) continue;
                    auto context = SetForScope(lhs_sim, *new_sim);
                    if (lattice.IsUnsupported(lhs_sims)) {
                        continue;
                    }
                    auto it = to_specialize.begin();
                    auto end = to_specialize.end();
                    for (; it != end; ++it) {
                        auto const& [index, old_bound] = *it;
                        if (index == lhs_spec_index) {
                            for (++it; it != end; ++it) {
                                auto const& [index, old_bound] = *it;
                                lattice.AddIfMinimalAndNotUnsupported(lhs_sims, old_bound, index);
                            }
                            break;
                        }
                        lattice.AddIfMinimalAndNotUnsupported(lhs_sims, old_bound, index);
                    }
                }
            } else {
                for (model::Index lhs_spec_index = 0; lhs_spec_index < col_matches_num;
                     ++lhs_spec_index) {
                    model::md::DecisionBoundary& lhs_sim = lhs_sims[lhs_spec_index];
                    std::optional<model::md::DecisionBoundary> const new_sim =
                            similarity_data.SpecializeOneLhs(lhs_spec_index, lhs_sim);
                    if (!new_sim.has_value()) continue;
                    auto context = SetForScope(lhs_sim, *new_sim);
                    if (lattice.IsUnsupported(lhs_sims)) {
                        continue;
                    }
                    auto it = to_specialize.begin();
                    auto end = to_specialize.end();
                    for (; it != end; ++it) {
                        auto const& [index, old_bound] = *it;
                        if (index == lhs_spec_index) {
                            if (old_bound > *new_sim) {
                                lattice.AddIfMinimalAndNotUnsupported(lhs_sims, old_bound, index);
                            }
                            for (++it; it != end; ++it) {
                                auto const& [index, old_bound] = *it;
                                lattice.AddIfMinimalAndNotUnsupported(lhs_sims, old_bound, index);
                            }
                            break;
                        }
                        lattice.AddIfMinimalAndNotUnsupported(lhs_sims, old_bound, index);
                    }
                }
            }
        }
        if (!traverse_all) return false;
        // TODO: if we specialized no LHSs, we can cut off the rest of the lattice here.
    }
    return true;
}

}  // namespace algos::hymd
