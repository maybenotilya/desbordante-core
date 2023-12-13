#include "algorithms/md/hymd/lattice_traverser.h"

#include <cassert>

#include <boost/asio.hpp>

#include "model/index.h"
#include "util/parallel_for.h"

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

class RhsTask {
    SimilarityData::ValidationResult result_;
    lattice::FullLattice& lattice_;
    lattice::ValidationInfo& info_;
    SimilarityData& similarity_data_;
    std::size_t min_support_;
    Recommendations& recommendations_;
    bool const prune_nondisjoint_;

public:
    RhsTask(lattice::FullLattice& lattice, lattice::ValidationInfo& info,
            SimilarityData& similarity_data, std::size_t min_support,
            Recommendations& recommendations, bool prune_nondisjoint)
        : lattice_(lattice),
          info_(info),
          similarity_data_(similarity_data),
          min_support_(min_support),
          recommendations_(recommendations),
          prune_nondisjoint_(prune_nondisjoint) {}

    void Validate() {
        result_ = similarity_data_.Validate(lattice_, info_, min_support_);
    }

    void AddViolations() {
        for (auto const& rhs_violations : result_.violations) {
            recommendations_.insert(rhs_violations.begin(), rhs_violations.end());
        }
    }

    void Specialize() {
        using model::md::DecisionBoundary;
        auto const& [violations, to_specialize, is_unsupported] = result_;
        for (auto const& [index, old_bound, actual_bound] : to_specialize) {
            info_.info->rhs_sims->operator[](index) = actual_bound;
        }
        if (is_unsupported) {
            lattice_.MarkUnsupported(info_.info->lhs_sims);
            return;
        }
        DecisionBoundaryVector& lhs_sims = info_.info->lhs_sims;
        std::size_t const col_matches_num = lhs_sims.size();
        auto specialize_all_lhs = [&](auto handle_same_lhs_as_rhs) {
            for (model::Index lhs_spec_index = 0; lhs_spec_index < col_matches_num;
                 ++lhs_spec_index) {
                DecisionBoundary& lhs_sim = lhs_sims[lhs_spec_index];
                std::optional<DecisionBoundary> const new_sim =
                        similarity_data_.SpecializeOneLhs(lhs_spec_index, lhs_sim);
                if (!new_sim.has_value()) continue;
                auto context = SetForScope(lhs_sim, *new_sim);
                if (lattice_.IsUnsupported(lhs_sims)) {
                    continue;
                }
                auto it = to_specialize.begin();
                auto end = to_specialize.end();
                for (; it != end; ++it) {
                    auto const& [rhs_index, old_rhs_bound, _] = *it;
                    if (rhs_index == lhs_spec_index) {
                        handle_same_lhs_as_rhs(old_rhs_bound, *new_sim, rhs_index);
                        for (++it; it != end; ++it) {
                            auto const& [rhs_index, old_rhs_bound, _] = *it;
                            lattice_.AddIfMinimalAndNotUnsupported(lhs_sims, old_rhs_bound,
                                                                   rhs_index);
                        }
                        break;
                    }
                    lattice_.AddIfMinimalAndNotUnsupported(lhs_sims, old_rhs_bound, rhs_index);
                }
            }
        };
        if (prune_nondisjoint_) {
            specialize_all_lhs([](...) {});
        } else {
            specialize_all_lhs([&](DecisionBoundary old_rhs_bound, DecisionBoundary new_bound,
                                   model::Index rhs_index) {
                if (old_rhs_bound > new_bound) {
                    lattice_.AddIfMinimalAndNotUnsupported(lhs_sims, old_rhs_bound, rhs_index);
                }
            });
        }
    }
};

bool LatticeTraverser::TraverseLattice(bool traverse_all) {
    SimilarityData& similarity_data = *similarity_data_;
    lattice::FullLattice& lattice = *lattice_;
    lattice::LevelGetter& level_getter = *level_getter_;
    while (level_getter.AreLevelsLeft()) {
        std::vector<lattice::ValidationInfo> mds = level_getter.GetCurrentMds();
        if (mds.empty()) {
            continue;
        }

        std::vector<RhsTask> tasks;
        tasks.reserve(mds.size());
        for (lattice::ValidationInfo& info : mds) {
            tasks.emplace_back(lattice, info, similarity_data, min_support_, *recommendations_ptr_,
                               prune_nondisjoint_);
        }
        // TODO: add reusable thread pool
        boost::asio::thread_pool thread_pool;
        for (RhsTask& task : tasks) {
            boost::asio::post(thread_pool, [&task]() { task.Validate(); });
        }
        thread_pool.join();
        auto viol_future = std::async([&tasks]() {
            for (RhsTask& task : tasks) {
                task.AddViolations();
            }
        });
        for (RhsTask& task : tasks) {
            task.Specialize();
        }
        viol_future.get();
        if (!traverse_all) return false;
        // TODO: if we specialized no LHSs, we can cut off the rest of the lattice here. (Can we?)
    }
    return true;
}

}  // namespace algos::hymd
