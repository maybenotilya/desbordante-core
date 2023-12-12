#include "algorithms/md/hymd/record_pair_inferrer.h"

#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "model/index.h"

namespace algos::hymd {

bool RecordPairInferrer::ShouldKeepInferring(Statistics const& statistics) const {
    // !(statistics.samples_done >= 2 || statistics.sim_vecs_processed > 100)
    return statistics.samples_done < 2 && statistics.sim_vecs_processed <= 100;
    /*
    return records_checked < 5 ||
           (mds_refined != 0 && records_checked / mds_refined < efficiency_reciprocal_);
    */
}

void RecordPairInferrer::ProcessSimVec(SimilarityVector const& sim) {
    std::vector<lattice::MdLatticeNodeInfo> violated_in_lattice = lattice_->FindViolated(sim);
    size_t const col_match_number = similarity_data_->GetColumnMatchNumber();
    for (lattice::MdLatticeNodeInfo& md : violated_in_lattice) {
        DecisionBoundaryVector& rhs_sims = *md.rhs_sims;
        DecisionBoundaryVector& lhs_sims = md.lhs_sims;
        for (model::Index rhs_index = 0; rhs_index < col_match_number; ++rhs_index) {
            preprocessing::Similarity const pair_rhs_sim = sim[rhs_index];
            model::md::DecisionBoundary const md_rhs_sim = rhs_sims[rhs_index];
            model::md::DecisionBoundary const lhs_sim_on_rhs = lhs_sims[rhs_index];
            assert(!(prune_nondisjoint_ && lhs_sim_on_rhs != 0.0) || md_rhs_sim == 0.0);
            if (pair_rhs_sim >= md_rhs_sim) continue;
            do {
                model::md::DecisionBoundary& md_rhs_sim_ref = rhs_sims[rhs_index];
                md_rhs_sim_ref = 0.0;
                // trivial
                // also cuts off MDs with similarity that is too low, since
                // only 0.0 can be a lower threshold than rhs_min_similarities[rhs_index],
                // everything else is cut off during index building.
                if (pair_rhs_sim <= lhs_sim_on_rhs) break;
                // supposed to be interesting at this point
                // not minimal
                if (lattice_->HasGeneralization(lhs_sims, pair_rhs_sim, rhs_index)) break;
                md_rhs_sim_ref = pair_rhs_sim;
            } while (false);
            auto const add_lhs_specialized_md = [this, &lhs_sims, md_rhs_sim, rhs_index,
                                                 &sim](model::Index i) {
                model::md::DecisionBoundary& lhs_sim = lhs_sims[i];
                std::optional<model::md::DecisionBoundary> new_lhs_value =
                        similarity_data_->SpecializeOneLhs(i, sim[i]);
                if (!new_lhs_value.has_value()) return;
                model::md::DecisionBoundary const old_lhs_sim = lhs_sim;
                lhs_sim = *new_lhs_value;
                lattice_->AddIfMinimalAndNotUnsupported(lhs_sims, md_rhs_sim, rhs_index);
                lhs_sim = old_lhs_sim;
            };
            for (model::Index i = 0; i < rhs_index; ++i) {
                add_lhs_specialized_md(i);
            }
            for (model::Index i = rhs_index + 1; i < col_match_number; ++i) {
                add_lhs_specialized_md(i);
            }
            if (!prune_nondisjoint_) {
                model::md::DecisionBoundary& lhs_sim = lhs_sims[rhs_index];
                std::optional<model::md::DecisionBoundary> new_lhs_value =
                        similarity_data_->SpecializeOneLhs(rhs_index, sim[rhs_index]);
                if (!new_lhs_value.has_value()) continue;
                model::md::DecisionBoundary const old_lhs_sim = lhs_sim;
                lhs_sim = *new_lhs_value;
                if (md_rhs_sim > lhs_sim) {
                    lattice_->AddIfMinimalAndNotUnsupported(lhs_sims, md_rhs_sim, rhs_index);
                }
                lhs_sim = old_lhs_sim;
            }
        }
    }
}

bool RecordPairInferrer::InferFromRecordPairs() {
    Statistics statistics;

    Recommendations& recommendations = *recommendations_ptr_;
    while (!recommendations.empty()) {
        auto it = recommendations.begin();
        Recommendation recommendation = *it;
        CompressedRecord const& left_record = *recommendation.left_record;
        CompressedRecord const& right_record = *recommendation.right_record;
        // All good, the above pointers point to the algorithm's index.
        recommendations.erase(it);
        SimilarityVector const sim =
                similarity_data_->GetSimilarityVector(left_record, right_record);
        if (avoid_same_sim_vec_processing_) {
            auto [_, not_seen] = checked_sim_vecs_.insert(sim);
            if (!not_seen) continue;
        }
        ProcessSimVec(sim);
        ++statistics.sim_vecs_processed;
        if (!ShouldKeepInferring(statistics)) {
            // efficiency_reciprocal_ *= 2;
            recommendations.clear();
            return false;
        }
    }
    while (!sim_vecs_to_check_.empty()) {
        auto node = sim_vecs_to_check_.extract(sim_vecs_to_check_.begin());
        SimilarityVector const sim = std::move(node.value());
        if (avoid_same_sim_vec_processing_) {
            auto [_, not_seen] = checked_sim_vecs_.insert(sim);
            if (!not_seen) continue;
        }
        ProcessSimVec(sim);
        ++statistics.sim_vecs_processed;
        if (!ShouldKeepInferring(statistics)) {
            // efficiency_reciprocal_ *= 2;
            return false;
        }
    }
    size_t const left_size = similarity_data_->GetLeftSize();
    while (cur_record_left_ < left_size) {
        std::unordered_set<SimilarityVector> sim_vecs =
                similarity_data_->GetSimVecs(cur_record_left_);
        while (!sim_vecs.empty()) {
            auto node = sim_vecs.extract(sim_vecs.begin());
            SimilarityVector const sim = std::move(node.value());
            if (avoid_same_sim_vec_processing_) {
                auto [_, not_seen] = checked_sim_vecs_.insert(sim);
                if (!not_seen) continue;
            }
            ProcessSimVec(sim);
            ++statistics.sim_vecs_processed;
            if (!ShouldKeepInferring(statistics)) {
                // efficiency_reciprocal_ *= 2;
                sim_vecs_to_check_ = std::move(sim_vecs);
                return false;
            }
        }
        ++cur_record_left_;
        ++statistics.samples_done;
    }
    return true;
}

}  // namespace algos::hymd
