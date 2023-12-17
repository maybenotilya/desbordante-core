#include "algorithms/md/hymd/lattice_traverser.h"

#include <boost/asio.hpp>

#include "algorithms/md/hymd/utility/set_for_scope.h"
#include "model/index.h"

namespace algos::hymd {

void LatticeTraverser::LowerAndSpecialize(SimilarityData::ValidationResult& validation_result,
                                          lattice::ValidationInfo& validation_info) {
    using model::md::DecisionBoundary, model::Index;
    auto const& [_, to_lower_info, is_unsupported] = validation_result;
    for (auto const& [index, _, actual_bound] : to_lower_info) {
        validation_info.node_info->rhs_bounds->operator[](index) = actual_bound;
    }
    // TODO: move the below to another class.
    if (is_unsupported) {
        lattice_->MarkUnsupported(validation_info.node_info->lhs_bounds);
        return;
    }
    DecisionBoundaryVector& lhs_bounds = validation_info.node_info->lhs_bounds;
    std::size_t const col_matches_num = lhs_bounds.size();
    auto specialize_all_lhs = [this, &to_lower_info, col_matches_num,
                               &lhs_bounds](auto handle_same_lhs_as_rhs) {
        for (Index lhs_spec_index = 0; lhs_spec_index < col_matches_num; ++lhs_spec_index) {
            DecisionBoundary& lhs_bound = lhs_bounds[lhs_spec_index];
            std::optional<DecisionBoundary> const specialized_lhs_bound =
                    similarity_data_->SpecializeOneLhs(lhs_spec_index, lhs_bound);
            if (!specialized_lhs_bound.has_value()) continue;
            auto context = utility::SetForScope(lhs_bound, *specialized_lhs_bound);
            if (lattice_->IsUnsupported(lhs_bounds)) {
                continue;
            }
            auto it = to_lower_info.begin();
            auto end = to_lower_info.end();
            for (; it != end; ++it) {
                auto const& [rhs_index, old_rhs_bound, _] = *it;
                if (rhs_index == lhs_spec_index) {
                    handle_same_lhs_as_rhs(old_rhs_bound, *specialized_lhs_bound, rhs_index);
                    for (++it; it != end; ++it) {
                        auto const& [rhs_index, old_rhs_bound, _] = *it;
                        lattice_->AddIfMinimal(lhs_bounds, old_rhs_bound, rhs_index);
                    }
                    break;
                }
                lattice_->AddIfMinimal(lhs_bounds, old_rhs_bound, rhs_index);
            }
        }
    };
    if (prune_nondisjoint_) {
        specialize_all_lhs([](...) {});
    } else {
        specialize_all_lhs([&](DecisionBoundary old_rhs_bound,
                               DecisionBoundary specialized_lhs_bound, Index rhs_index) {
            if (old_rhs_bound > specialized_lhs_bound) {
                lattice_->AddIfMinimalAndNotUnsupported(lhs_bounds, old_rhs_bound, rhs_index);
            }
        });
    }
}

bool LatticeTraverser::TraverseLattice(bool traverse_all) {
    while (level_getter_->AreLevelsLeft()) {
        std::vector<lattice::ValidationInfo> mds = level_getter_->GetCurrentMds();
        if (mds.empty()) {
            continue;
        }

        std::size_t const mds_size = mds.size();
        std::vector<SimilarityData::ValidationResult> results(mds_size);
        // TODO: add reusable thread pool
        boost::asio::thread_pool thread_pool;
        for (model::Index i = 0; i < mds_size; ++i) {
            boost::asio::post(thread_pool, [this, &result = results[i], &info = mds[i]]() {
                result = similarity_data_->Validate(info);
            });
        }
        thread_pool.join();
        auto viol_future = std::async(std::launch::async, [this, &results]() {
            for (SimilarityData::ValidationResult& result : results) {
                for (std::vector<Recommendation> const& rhs_violations : result.recommendations) {
                    recommendations_.insert(rhs_violations.begin(), rhs_violations.end());
                };
            }
        });
        for (model::Index i = 0; i < mds_size; ++i) {
            LowerAndSpecialize(results[i], mds[i]);
        }
        viol_future.get();
        if (!traverse_all) return false;
    }
    return true;
}

}  // namespace algos::hymd
