#include "algorithms/md/hymd/record_pair_inferrer.h"

namespace algos::hymd {

bool RecordPairInferrer::ShouldKeepInferring(size_t records_checked, size_t mds_refined) const {
    return records_checked < 5 ||
           (mds_refined != 0 && records_checked / mds_refined < efficiency_reciprocal_);
}

size_t RecordPairInferrer::CheckRecordPair(size_t left_record, size_t right_record,
                                           bool prune_nondisjoint) {
    model::SimilarityVector sim = similarity_data_->GetSimilarityVector(left_record, right_record);
    std::vector<model::LatticeMd> violated = lattice_->FindViolated(sim);
    model::SimilarityVector const& rhs_min_similarities = similarity_data_->GetRhsMinSimilarities();
    size_t const col_match_number = similarity_data_->GetColumnMatchNumber();
    for (model::LatticeMd const& md : violated) {
        lattice_->RemoveMd(md);
        size_t const rhs_index = md.rhs_index;
        model::Similarity const rec_rhs_sim = sim[rhs_index];
        model::SimilarityVector const& md_lhs = md.lhs_sims;
        model::Similarity const md_lhs_on_rhs = md_lhs[rhs_index];
        if (rec_rhs_sim >= rhs_min_similarities[rhs_index] && rec_rhs_sim > md_lhs[rhs_index]) {
            if (!prune_nondisjoint || md_lhs_on_rhs == 0.0)
                lattice_->AddIfMinAndNotUnsupported({md.lhs_sims, rec_rhs_sim, rhs_index});
        }
        for (size_t i = 0; i < col_match_number; ++i) {
            if (prune_nondisjoint && i == rhs_index) continue;
            std::optional<model::SimilarityVector> const& new_lhs =
                    similarity_data_->SpecializeLhs(md_lhs, i, sim[i]);
            if (!new_lhs.has_value()) continue;
            if (md.rhs_sim > new_lhs.value()[md.rhs_index]) {
                lattice_->AddIfMinAndNotUnsupported({new_lhs.value(), md.rhs_sim, rhs_index});
            }
        }
    }
    return violated.size();
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
