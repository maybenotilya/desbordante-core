#include "algorithms/md/hymd/lattice/cardinality/one_by_one_min_picker.h"

#include <algorithm>
#include <cassert>

namespace algos::hymd::lattice::cardinality {

void OneByOnePicker::NewBatch(std::size_t elements) {
    currently_picked_.clear();
    currently_picked_.reserve(elements);
}

void OneByOnePicker::AddGeneralizations(MdLatticeNodeInfo& md,
                                        std::unordered_set<model::Index>& considered_indices) {
    DecisionBoundaryVector const& lhs_sims_cur = md.lhs_sims;
    auto cur_end = lhs_sims_cur.end();
    for (ValidationInfo& prev_info : currently_picked_) {
        DecisionBoundaryVector const& prev = prev_info.info->lhs_sims;
        std::unordered_set<model::Index>& prev_indices = prev_info.rhs_indices;
        for (auto cur_it = lhs_sims_cur.begin(), prev_it = prev.begin(); cur_it != cur_end;
             ++cur_it, ++prev_it) {
            model::md::DecisionBoundary const cur_bound = *cur_it;
            model::md::DecisionBoundary const prev_bound = *prev_it;
            if (cur_bound < prev_bound) {
                for (++cur_it, ++prev_it; cur_it != cur_end; ++cur_it, ++prev_it) {
                    model::md::DecisionBoundary const cur_bound = *cur_it;
                    model::md::DecisionBoundary const prev_bound = *prev_it;
                    if (cur_bound > prev_bound) {
                        goto incomparable;
                    }
                }
                if (prev_indices.size() < considered_indices.size()) {
                    std::erase_if(prev_indices,
                                  [&considered_indices,
                                   cons_end = considered_indices.end()](model::Index index) {
                                      return considered_indices.find(index) != cons_end;
                                  });
                } else {
                    std::for_each(
                            considered_indices.begin(), considered_indices.end(),
                            [&prev_indices](model::Index index) { prev_indices.erase(index); });
                }
            } else if (cur_bound > prev_bound) {
                for (++cur_it, ++prev_it; cur_it != cur_end; ++cur_it, ++prev_it) {
                    model::md::DecisionBoundary const cur_bound = *cur_it;
                    model::md::DecisionBoundary const prev_bound = *prev_it;
                    if (cur_bound < prev_bound) {
                        goto incomparable;
                    }
                }
                if (considered_indices.size() < prev_indices.size()) {
                    std::erase_if(
                            considered_indices,
                            [&prev_indices, prev_end = prev_indices.end()](model::Index index) {
                                return prev_indices.find(index) != prev_end;
                            });
                } else {
                    std::for_each(prev_indices.begin(), prev_indices.end(),
                                  [&considered_indices](model::Index index) {
                                      considered_indices.erase(index);
                                  });
                }
                if (considered_indices.empty()) return;
            }
        }
    incomparable:;
    }
    assert(!considered_indices.empty());
    currently_picked_.emplace_back(&md, std::move(considered_indices));
}

std::vector<ValidationInfo> OneByOnePicker::GetAll() {
    static_assert(kNeedsEmptyRemoval,
                  "This picker needs post-processing to remove candidates with empty RHS indices");
    return std::move(currently_picked_);
}

}  // namespace algos::hymd::lattice::cardinality
