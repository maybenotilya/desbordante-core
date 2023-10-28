#include "algorithms/md/hymd/record_pair_inferrer.h"

namespace algos::hymd {

bool RecordPairInferrer::ShouldKeepInferring(size_t records_checked, size_t mds_refined) const {
    return records_checked < 5 ||
           (mds_refined != 0 && records_checked / mds_refined < efficiency_reciprocal_);
}

size_t RecordPairInferrer::CheckRecordPair(size_t left_record, size_t right_record,
                                           bool prune_nondisjoint) {
    model::SimilarityVector sim = similarity_data_->GetSimilarityVector(left_record, right_record);
    std::vector<model::MdLatticeNodeInfo> violated_in_lattice = lattice_->FindViolated(sim);
    size_t violated_mds = 0;
    model::SimilarityVector const& rhs_min_similarities = similarity_data_->GetRhsMinSimilarities();
    size_t const col_match_number = similarity_data_->GetColumnMatchNumber();
    for (model::MdLatticeNodeInfo const& md : violated_in_lattice) {
        model::SimilarityVector& rhs_sims = *md.rhs_sims;
        model::SimilarityVector lhs_sims = md.lhs_sims;
        for (size_t rhs_index = 0; rhs_index < col_match_number; ++rhs_index) {
            model::Similarity const pair_rhs_sim = sim[rhs_index];
            model::Similarity const md_rhs_sim = rhs_sims[rhs_index];
            model::Similarity const lhs_sim_on_rhs = lhs_sims[rhs_index];
            assert(!(prune_nondisjoint && lhs_sim_on_rhs != 0.0) || md_rhs_sim == 0.0);
            if (pair_rhs_sim >= md_rhs_sim) continue;
            ++violated_mds;
            do {
                model::Similarity& md_rhs_sim_ref = rhs_sims[rhs_index];
                md_rhs_sim_ref = 0.0;
                // trivial
                // also cuts off MDs with similarity that is too low, since
                // only 0.0 can be a lower threshold than rhs_min_similarities[rhs_index],
                // everything else is cut off during index building.
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
                lattice_->AddIfMinAndNotUnsupported(lhs_sims, md_rhs_sim, rhs_index);
                lhs_sim = old_lhs_sim;
            };
            for (size_t i = 0; i < rhs_index; ++i) {
                add_lhs_specialized_md(i);
            }
            for (size_t i = rhs_index + 1; i < col_match_number; ++i) {
                add_lhs_specialized_md(i);
            }
            if (!prune_nondisjoint) {
                model::Similarity& lhs_sim = lhs_sims[rhs_index];
                std::optional<model::Similarity> new_lhs_value =
                        similarity_data_->SpecializeOneLhs(rhs_index, sim[rhs_index]);
                if (!new_lhs_value.has_value()) continue;
                model::Similarity const old_lhs_sim = lhs_sim;
                lhs_sim = new_lhs_value.value();
                if (md_rhs_sim > lhs_sim) {
                    lattice_->AddIfMinAndNotUnsupported(lhs_sims, md_rhs_sim, rhs_index);
                }
                lhs_sim = old_lhs_sim;
            }
        }
    }
    return violated_mds;
}

bool RecordPairInferrer::InferFromRecordPairs() {
    size_t records_checked = 0;
    size_t mds_refined = 0;

    Recommendations& recommendations = *recommendations_ptr_;
    while (!recommendations.empty()) {
        auto it = recommendations.begin();
        std::pair<size_t, size_t> rec_pair = *it;
        recommendations.erase(it);
        auto const [left_record, right_record] = rec_pair;
        mds_refined += CheckRecordPair(left_record, right_record);
        checked_recommendations_.emplace(rec_pair);
        ++records_checked;
        if (!ShouldKeepInferring(records_checked, mds_refined)) {
            efficiency_reciprocal_ *= 2;
            recommendations.clear();
            return false;
        }
    }
    size_t const left_size = similarity_data_->GetLeftSize();
    size_t const right_size = similarity_data_->GetRightSize();
    while (cur_record_left_ < left_size) {
        while (cur_record_right_ < right_size) {
            if (checked_recommendations_.find({cur_record_left_, cur_record_right_}) !=
                checked_recommendations_.end()) {
                ++cur_record_right_;
                continue;
            }
            mds_refined += CheckRecordPair(cur_record_left_, cur_record_right_);
            ++cur_record_right_;
            ++records_checked;
            if (!ShouldKeepInferring(records_checked, mds_refined)) {
                efficiency_reciprocal_ *= 2;
                return false;
            }
        }
        cur_record_right_ = 0;
        ++cur_record_left_;
    }
    return true;
}

}
