#include "algorithms/md/hymd/record_pair_inferrer.h"

#include <cstddef>

#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/utility/set_for_scope.h"
#include "model/index.h"

namespace algos::hymd {

struct RecordPairInferrer::Statistics {
    std::size_t samplings_started = 0;
    std::size_t sim_vecs_processed = 0;
};

bool RecordPairInferrer::ShouldStopInferring(Statistics const& statistics) const {
    return statistics.samplings_started >= 2 || statistics.sim_vecs_processed > 100;
    /*
    return records_checked >= 5 &&
           (mds_refined == 0 || records_checked / mds_refined >= efficiency_reciprocal_);
    */
}

void RecordPairInferrer::ProcessSimVec(SimilarityVector const& sim) {
    using model::md::DecisionBoundary, model::Index;
    std::vector<lattice::MdLatticeNodeInfo> violated_in_lattice = lattice_->FindViolated(sim);
    std::size_t const col_match_number = similarity_data_->GetColumnMatchNumber();
    for (lattice::MdLatticeNodeInfo& md : violated_in_lattice) {
        DecisionBoundaryVector& rhs_sims = *md.rhs_bounds;
        DecisionBoundaryVector& lhs_sims = md.lhs_bounds;
        for (Index rhs_index = 0; rhs_index < col_match_number; ++rhs_index) {
            preprocessing::Similarity const pair_rhs_bound = sim[rhs_index];
            DecisionBoundary const old_md_rhs_bound = rhs_sims[rhs_index];
            DecisionBoundary const md_lhs_bound_on_rhs = lhs_sims[rhs_index];
            assert(!(prune_nondisjoint_ && md_lhs_bound_on_rhs != 0.0) || old_md_rhs_bound == 0.0);
            if (pair_rhs_bound >= old_md_rhs_bound) continue;
            do {
                DecisionBoundary& md_rhs_bound_ref = rhs_sims[rhs_index];
                md_rhs_bound_ref = 0.0;
                // trivial
                if (pair_rhs_bound <= md_lhs_bound_on_rhs) break;
                // not minimal
                if (lattice_->HasGeneralization(lhs_sims, pair_rhs_bound, rhs_index)) break;
                md_rhs_bound_ref = pair_rhs_bound;
            } while (false);
            // TODO: move the below to another class.
            auto const add_lhs_specialized_md = [this, &lhs_sims, old_md_rhs_bound, rhs_index,
                                                 &sim](Index i, auto should_add) {
                DecisionBoundary& lhs_sim = lhs_sims[i];
                std::optional<DecisionBoundary> new_lhs_value =
                        similarity_data_->SpecializeOneLhs(i, sim[i]);
                if (!new_lhs_value.has_value()) return;
                auto context = utility::SetForScope(lhs_sim, *new_lhs_value);
                if (should_add(old_md_rhs_bound, lhs_sim)) {
                    lattice_->AddIfMinimalAndNotUnsupported(lhs_sims, old_md_rhs_bound, rhs_index);
                }
            };
            auto true_func = [](...) { return true; };
            for (Index i = 0; i < rhs_index; ++i) {
                add_lhs_specialized_md(i, true_func);
            }
            for (Index i = rhs_index + 1; i < col_match_number; ++i) {
                add_lhs_specialized_md(i, true_func);
            }
            if (!prune_nondisjoint_) {
                add_lhs_specialized_md(rhs_index, std::greater<>{});
            }
        }
    }
}

bool RecordPairInferrer::InferFromRecordPairs(Recommendations recommendations) {
    Statistics statistics;

    auto process_collection = [&](auto& collection, auto get_sim_vec) {
        while (!collection.empty()) {
            SimilarityVector const sim =
                    get_sim_vec(collection.extract(collection.begin()).value());
            if (avoid_same_sim_vec_processing_) {
                auto [_, not_seen] = checked_sim_vecs_.insert(sim);
                if (!not_seen) continue;
            }
            ProcessSimVec(sim);
            ++statistics.sim_vecs_processed;
            if (ShouldStopInferring(statistics)) {
                // efficiency_reciprocal_ *= 2;
                return true;
            }
        }
        return false;
    };
    if (process_collection(recommendations, [&](Recommendation& rec) {
            // TODO: parallelize similarity vector calculation
            return similarity_data_->GetSimilarityVector(*rec.left_record, *rec.right_record);
        })) {
        return false;
    }
    auto move_out = [&](SimilarityVector& v) { return std::move(v); };
    if (process_collection(sim_vecs_to_check_, move_out)) {
        return false;
    }
    std::size_t const left_size = similarity_data_->GetLeftSize();
    while (next_left_record_ < left_size) {
        ++statistics.samplings_started;
        sim_vecs_to_check_ = similarity_data_->GetSimVecs(next_left_record_);
        ++next_left_record_;
        if (process_collection(sim_vecs_to_check_, move_out)) {
            return false;
        }
    }
    return true;
}

}  // namespace algos::hymd
