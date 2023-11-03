#include "algorithms/md/hymd/record_pair_inferrer.h"

namespace algos::hymd {

bool RecordPairInferrer::ShouldKeepInferring(Statistics const& statistics) const {
    // !(statistics.samples_done >= 2 || statistics.sim_vecs_processed > 100)
    return statistics.samples_done < 2 && statistics.sim_vecs_processed <= 100;
    /*
    return records_checked < 5 ||
           (mds_refined != 0 && records_checked / mds_refined < efficiency_reciprocal_);
    */
}

void RecordPairInferrer::ProcessSimVec(DecisionBoundsVector const& sim) {
    std::vector<model::MdLatticeNodeInfo> violated_in_lattice = lattice_->FindViolated(sim);
    model::SimilarityVector const& rhs_min_similarities = similarity_data_->GetRhsMinSimilarities();
    size_t const col_match_number = similarity_data_->GetColumnMatchNumber();
    for (model::MdLatticeNodeInfo const& md : violated_in_lattice) {
        model::SimilarityVector& rhs_sims = *md.rhs_sims;
        model::SimilarityVector lhs_sims = md.lhs_sims;
        for (size_t rhs_index = 0; rhs_index < col_match_number; ++rhs_index) {
            model::Similarity const pair_rhs_sim = sim[rhs_index];
            model::Similarity const md_rhs_sim = rhs_sims[rhs_index];
            model::Similarity const lhs_sim_on_rhs = lhs_sims[rhs_index];
            assert(!(prune_nondisjoint_ && lhs_sim_on_rhs != 0.0) || md_rhs_sim == 0.0);
            if (pair_rhs_sim >= md_rhs_sim) continue;
            do {
                model::Similarity& md_rhs_sim_ref = rhs_sims[rhs_index];
                md_rhs_sim_ref = 0.0;
                // trivial
                // also cuts off MDs with similarity that is too low, since
                // only 0.0 can be a lower threshold than rhs_min_similarities[rhs_index],
                // everything else is cut off during index building.
                // TODO: mark as disproved so that lattice traversal doesn't have to look
                //   through indexes for this column match. Set to -1.0?
                if (pair_rhs_sim <= lhs_sim_on_rhs) break;
                // not minimal
                if (lattice_->HasGeneralization(lhs_sims, pair_rhs_sim, rhs_index)) break;
                // supposed to be interesting at this point
                assert(pair_rhs_sim >= rhs_min_similarities[rhs_index]);
                md_rhs_sim_ref = pair_rhs_sim;
            } while(false);
            auto const add_lhs_specialized_md = [this, &lhs_sims, md_rhs_sim, rhs_index,
                                                 &sim](size_t i) {
                model::Similarity& lhs_sim = lhs_sims[i];
                std::optional<model::Similarity> new_lhs_value =
                        similarity_data_->SpecializeOneLhs(i, sim[i]);
                if (!new_lhs_value.has_value()) return;
                model::Similarity const old_lhs_sim = lhs_sim;
                lhs_sim = new_lhs_value.value();
                lattice_->AddIfMinimalAndNotUnsupported(lhs_sims, md_rhs_sim, rhs_index);
                lhs_sim = old_lhs_sim;
            };
            for (size_t i = 0; i < rhs_index; ++i) {
                add_lhs_specialized_md(i);
            }
            for (size_t i = rhs_index + 1; i < col_match_number; ++i) {
                add_lhs_specialized_md(i);
            }
            if (!prune_nondisjoint_) {
                model::Similarity& lhs_sim = lhs_sims[rhs_index];
                std::optional<model::Similarity> new_lhs_value =
                        similarity_data_->SpecializeOneLhs(rhs_index, sim[rhs_index]);
                if (!new_lhs_value.has_value()) continue;
                model::Similarity const old_lhs_sim = lhs_sim;
                lhs_sim = new_lhs_value.value();
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
    // TODO: what is std::_Hashtable<
    // unsigned long,
    // std::pair<unsigned long const, double>,
    // std::allocator<std::pair<unsigned long const, double> >,
    // std::__detail::_Select1st,
    // std::equal_to<unsigned long>,
    // std::hash<unsigned long>,
    // std::__detail::_Mod_range_hashing,
    // std::__detail::_Default_ranged_hash,
    // std::__detail::_Prime_rehash_policy,
    // std::__detail::_Hashtable_traits<false, false, true>>
    //   ::find(unsigned long const&) const
    // ?

    Recommendations& recommendations = *recommendations_ptr_;
    while (!recommendations.empty()) {
        auto it = recommendations.begin();
        Recommendation recommendation = *it;
        CompressedRecord const& left_record = *recommendation.left_record;
        CompressedRecord const& right_record = *recommendation.right_record;
        // All good, the above pointers point to the algorithm's index.
        recommendations.erase(it);
        auto const sim = similarity_data_->GetSimilarityVector(left_record, right_record);
        if (avoid_same_sim_vec_processing_) {
            auto [_, not_seen] = checked_sim_vecs_.insert(sim);
            if (!not_seen) continue;
        }
        ProcessSimVec(sim);
        ++statistics.sim_vecs_processed;
        if (!ShouldKeepInferring(statistics)) {
            //efficiency_reciprocal_ *= 2;
            recommendations.clear();
            return false;
        }
    }
    while (!sim_vecs_to_check_.empty()) {
        auto node = sim_vecs_to_check_.extract(sim_vecs_to_check_.begin());
        DecisionBoundsVector const sim = std::move(node.value());
        if (avoid_same_sim_vec_processing_) {
            auto [_, not_seen] = checked_sim_vecs_.insert(sim);
            if (!not_seen) continue;
        }
        ProcessSimVec(sim);
        ++statistics.sim_vecs_processed;
        if (!ShouldKeepInferring(statistics)) {
            //efficiency_reciprocal_ *= 2;
            return false;
        }
    }
    size_t const left_size = similarity_data_->GetLeftSize();
    while (cur_record_left_ < left_size) {
        DecBoundVectorUnorderedSet sim_vecs = similarity_data_->GetSimVecs(cur_record_left_);
        while (!sim_vecs.empty()) {
            auto node = sim_vecs.extract(sim_vecs.begin());
            DecisionBoundsVector const sim = std::move(node.value());
            if (avoid_same_sim_vec_processing_) {
                auto [_, not_seen] = checked_sim_vecs_.insert(sim);
                if (!not_seen) continue;
            }
            ProcessSimVec(sim);
            ++statistics.sim_vecs_processed;
            if (!ShouldKeepInferring(statistics)) {
                //efficiency_reciprocal_ *= 2;
                sim_vecs_to_check_ = std::move(sim_vecs);
                return false;
            }
        }
        ++cur_record_left_;
        ++statistics.samples_done;
    }
    return true;
}

}
