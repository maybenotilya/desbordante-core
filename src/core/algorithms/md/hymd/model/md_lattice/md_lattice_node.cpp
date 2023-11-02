#include "md_lattice_node.h"

#include <algorithm>
#include <cassert>

#include "algorithms/md/hymd/model/get_first_non_zero_index.h"

namespace algos::hymd::model {

void MdLatticeNode::AddUnchecked(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                 size_t rhs_index, size_t const this_node_index) {
    assert(children_.empty());
    assert(std::all_of(rhs_.begin(), rhs_.end(), [](Similarity const v) { return v == 0.0; }));
    size_t const rhs_size = rhs_.size();
    MdLatticeNode* cur_node_ptr = this;
    for (size_t cur_node_index = util::GetFirstNonZeroIndex(lhs_sims, this_node_index);
         cur_node_index != rhs_size;
         cur_node_index = util::GetFirstNonZeroIndex(lhs_sims, cur_node_index + 1)) {
        cur_node_ptr = (cur_node_ptr->children_[cur_node_index][lhs_sims[cur_node_index]] =
                                std::make_unique<MdLatticeNode>(rhs_size))
                               .get();
    }
    cur_node_ptr->rhs_[rhs_index] = rhs_sim;
}

void MdLatticeNode::Add(SimilarityVector const& lhs_sims, Similarity rhs_sim, size_t rhs_index,
                        size_t const this_node_index) {
    size_t const rhs_size = rhs_.size();
    MdLatticeNode* cur_node_ptr = this;
    for (size_t cur_node_index = util::GetFirstNonZeroIndex(lhs_sims, this_node_index);
         cur_node_index != rhs_size;
         cur_node_index = util::GetFirstNonZeroIndex(lhs_sims, cur_node_index + 1)) {
        model::Similarity const child_similarity = lhs_sims[cur_node_index];
        auto [it_arr, is_first_arr] = cur_node_ptr->children_.try_emplace(cur_node_index);
        ThresholdMap& child_threshold_map = it_arr->second;
        if (is_first_arr) [[unlikely]] {
            (child_threshold_map[child_similarity] = std::make_unique<MdLatticeNode>(rhs_size))
                ->AddUnchecked(lhs_sims, rhs_sim, rhs_index, cur_node_index + 1);
            return;
        }
        auto [it_map, is_first_map] = child_threshold_map.try_emplace(child_similarity);
        std::unique_ptr<MdLatticeNode>& node_ptr = it_map->second;
        if (is_first_map) /*?? [[unlikely]] ??*/ {
            assert(node_ptr == nullptr);
            (node_ptr = std::make_unique<MdLatticeNode>(rhs_size))
                    ->AddUnchecked(lhs_sims, rhs_sim, rhs_index, cur_node_index + 1);
            return;
        }
        assert(node_ptr != nullptr);
        cur_node_ptr = node_ptr.get();
    }
    double& cur_sim = cur_node_ptr->rhs_[rhs_index];
    if (cur_sim == 0.0 || rhs_sim < cur_sim) cur_sim = rhs_sim;
}

bool MdLatticeNode::HasGeneralization(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                      size_t rhs_index, size_t this_node_index) const {
    if (rhs_[rhs_index] >= rhs_sim) return true;
    size_t const rhs_size = rhs_.size();
    for (size_t cur_index = util::GetFirstNonZeroIndex(lhs_sims, this_node_index);
         cur_index != rhs_size; cur_index = util::GetFirstNonZeroIndex(lhs_sims, cur_index + 1)) {
        auto it = children_.find(cur_index);
        if (it == children_.end()) continue;
        Similarity const child_similarity = lhs_sims[cur_index];
        for (auto const& [threshold, node] : it->second) {
            if (threshold > child_similarity) break;
            if (node->HasGeneralization(lhs_sims, rhs_sim, rhs_index, cur_index + 1)) return true;
        }
    }
    return false;
}

void MdLatticeNode::AddIfMinimal(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                 size_t rhs_index, size_t const this_node_index) {
    double& this_rhs_sim = rhs_[rhs_index];
    if (this_rhs_sim >= rhs_sim) return;
    size_t const rhs_size = rhs_.size();
    size_t const next_node_index = util::GetFirstNonZeroIndex(lhs_sims, this_node_index);
    if (next_node_index == rhs_size) {
        if (this_rhs_sim == 0.0) {
            this_rhs_sim = rhs_sim;
        }
        return;
    }
    {
        auto children_end = children_.end();
        for (size_t child_node_index = util::GetFirstNonZeroIndex(lhs_sims, next_node_index + 1);
             child_node_index != rhs_size;
             child_node_index = util::GetFirstNonZeroIndex(lhs_sims, child_node_index + 1)) {
            auto it = children_.find(child_node_index);
            if (it == children_end) continue;
            Similarity const child_lhs_sim = lhs_sims[child_node_index];
            for (auto const& [threshold, node] : it->second) {
                if (threshold > child_lhs_sim) break;
                if (node->HasGeneralization(lhs_sims, rhs_sim, rhs_index, child_node_index + 1))
                    return;
            }
        }
    }
    auto [it_arr, is_first_arr] = children_.try_emplace(next_node_index);
    ThresholdMap& threshold_mapping = it_arr->second;
    Similarity const next_lhs_sim = lhs_sims[next_node_index];
    if (is_first_arr) [[unlikely]] {
        (threshold_mapping[next_lhs_sim] = std::make_unique<MdLatticeNode>(rhs_size))
                ->AddUnchecked(lhs_sims, rhs_sim, rhs_index, next_node_index + 1);
        return;
    }
    for (auto const& [threshold, node] : threshold_mapping) {
        if (threshold > next_lhs_sim) {
            break;
        }
        if (threshold == next_lhs_sim) {
            node->AddIfMinimal(lhs_sims, rhs_sim, rhs_index, next_node_index + 1);
            return;
        }
        if (node->HasGeneralization(lhs_sims, rhs_sim, rhs_index, next_node_index + 1)) {
            return;
        }
    }
    (threshold_mapping[next_lhs_sim] = std::make_unique<MdLatticeNode>(rhs_size))
                ->AddUnchecked(lhs_sims, rhs_sim, rhs_index, next_node_index + 1);
}

void MdLatticeNode::RemoveNode(SimilarityVector const& lhs_vec, size_t this_node_index) {
    assert(this_node_index <= lhs_vec.size());
    size_t const next_node_index = util::GetFirstNonZeroIndex(lhs_vec, this_node_index);
    if (next_node_index == lhs_vec.size()) {
        rhs_.assign(rhs_.size(), 0.0);
        return;
    }
    assert(next_node_index < lhs_vec.size());
    model::Similarity const child_similarity = lhs_vec[next_node_index];
    ThresholdMap& threshold_map = children_[next_node_index];
    std::unique_ptr<MdLatticeNode>& node_ptr = threshold_map[child_similarity];
    assert(node_ptr != nullptr);
    node_ptr->RemoveNode(lhs_vec, next_node_index + 1);
}

void MdLatticeNode::FindViolated(std::vector<MdLatticeNodeInfo>& found,
                                 SimilarityVector& this_node_lhs,
                                 SimilarityVector const& similarity_vector,
                                 [[maybe_unused]] size_t this_node_index) {
    {
        /*
        size_t const col_matches = similarity_vector.size()
        for (size_t i = 0; i < col_matches; ++i) {
            if (similarity_vector[i] < rhs_[i]) {
                found.emplace_back(this_node_lhs, &rhs_);
                break;
            }
        }
        */
        assert(rhs_.size() == similarity_vector.size());
        auto it_rhs = rhs_.begin();
        auto it_sim = similarity_vector.begin();
        auto end_rhs = rhs_.end();
        for (; it_rhs != end_rhs; ++it_rhs, ++it_sim) {
            if (*it_sim < *it_rhs) {
                found.emplace_back(this_node_lhs, &rhs_);
                break;
            }
        }
    }
    for (auto const& [index, threshold_mapping] : children_) {
        Similarity& cur_lhs_sim = this_node_lhs[index];
        Similarity const sim_vec_sim = similarity_vector[index];
        assert(index < similarity_vector.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            if (threshold > sim_vec_sim) {
                break;
            }
            cur_lhs_sim = threshold;
            node->FindViolated(found, this_node_lhs, similarity_vector, index + 1);
        }
        cur_lhs_sim = 0.0;
    }
}

void MdLatticeNode::GetMaxValidGeneralizationRhs(SimilarityVector const& lhs,
                                                 SimilarityVector& cur_rhs,
                                                 size_t this_node_index) const {
    for (size_t i = 0; i < rhs_.size(); ++i) {
        double const rhs_threshold = rhs_[i];
        double& cur_rhs_threshold = cur_rhs[i];
        if (rhs_threshold > cur_rhs_threshold) {
            cur_rhs_threshold = rhs_threshold;
        }
    }

    // TODO: wtf was I on when I wrote this?
    auto const& child_array_end = children_.end();
    for (size_t cur_node_index = this_node_index;
         (cur_node_index = util::GetFirstNonZeroIndex(lhs, cur_node_index)) != lhs.size();
         ++cur_node_index) {
        auto it = children_.find(cur_node_index);
        if (it == child_array_end) continue;
        for (auto const& [threshold, node] : it->second) {
            if (threshold > lhs[cur_node_index]) {
                break;
            }
            node->GetMaxValidGeneralizationRhs(lhs, cur_rhs, cur_node_index + 1);
        }
    }
}

void MdLatticeNode::GetLevel(std::vector<LatticeNodeSims>& collected,
                             SimilarityVector& this_node_lhs,
                             [[maybe_unused]] size_t this_node_index, size_t sims_left) const {
    if (sims_left == 0) {
        if (std::any_of(rhs_.begin(), rhs_.end(), [](Similarity val) { return val != 0.0; }))
            collected.emplace_back(this_node_lhs, rhs_);
        return;
    }
    for (auto const& [index, threshold_mapping] : children_) {
        assert(index < this_node_lhs.size());
        for (auto const& [threshold, node] : threshold_mapping) {
            assert(threshold > 0.0);
            this_node_lhs[index] = threshold;
            node->GetLevel(collected, this_node_lhs, index + 1, sims_left - 1);
        }
        this_node_lhs[index] = 0.0;
    }
}

}
