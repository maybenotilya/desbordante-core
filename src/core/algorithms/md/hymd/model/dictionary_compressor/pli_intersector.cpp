#include "algorithms/md/hymd/model/dictionary_compressor/pli_intersector.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include "util/intersect_sorted_sequences.h"

namespace {
using algos::hymd::model::KeyedPositionListIndex;
std::vector<size_t> GetEndingValueIds(std::vector<KeyedPositionListIndex const*> const& plis) {
    std::vector<size_t> value_ids;
    value_ids.reserve(plis.size());
    for (auto const* pli : plis) {
        value_ids.push_back(pli->GetClusters().size() - 1);
    }
    ++value_ids[value_ids.size() - 1];
    return value_ids;
}
std::vector<size_t> GetPliSizes(std::vector<KeyedPositionListIndex const*> const& plis) {
    std::vector<size_t> pli_sizes;
    pli_sizes.reserve(plis.size());
    for (KeyedPositionListIndex const* pli_ptr : plis) {
        pli_sizes.push_back(pli_ptr->GetClusters().size());
    }
    return pli_sizes;
}
}

namespace algos::hymd::model {

using iter = PliIntersector::const_iterator;

PliIntersector::PliIntersector(std::vector<KeyedPositionListIndex const*> plis)
    : plis_(std::move(plis)), end_iter_(&plis_) {}

iter::value_type iter::operator*() {
    return {value_ids_, intersection_};
}

bool operator!=(iter const& a, iter const& b) {
    return !(a == b);
}

bool operator==(iter const& a, iter const& b) {
    return a.plis_ == b.plis_ && a.value_ids_ == b.value_ids_ && a.intersection_ == b.intersection_;
}

iter::const_iterator(std::vector<KeyedPositionListIndex const*> const* plis,
                     std::vector<size_t> value_ids)
    : plis_(plis),
      pli_num_(plis->size()),
      pli_sizes_(GetPliSizes(*plis)),
      value_ids_(std::move(value_ids)) {
    assert(!plis_->empty());
    assert(ValueIdsAreValid());
    GetCluster();
    if (intersection_.empty()) ++*this;
}

iter::const_iterator(std::vector<KeyedPositionListIndex const*> const* plis)
    : plis_(plis),
      pli_num_(plis->size()),
      pli_sizes_(GetPliSizes(*plis)),
      value_ids_(GetEndingValueIds(*plis)) {}

bool iter::ValueIdsAreValid() {
    for (size_t i = 0; i < pli_num_; ++i) {
        if (pli_sizes_[i] <= value_ids_[i]) return false;
    }
    return true;
}

iter const& PliIntersector::end() const {
    return end_iter_;
}

iter PliIntersector::begin() const {
    return {&plis_, std::vector<size_t>(plis_.size(), 0)};
}

iter& iter::operator++() {
    while (IncValueIds()) {
        GetCluster();
        if (!intersection_.empty()) return *this;
    }
    intersection_.clear();
    return *this;
}

bool iter::IncValueIds() {
    assert(ValueIdsAreValid());
    assert(pli_num_ != 0);
    size_t cur_index = pli_num_ - 1;
    ++value_ids_[cur_index];
    if (value_ids_[cur_index] >= pli_sizes_[cur_index]) {
        do {
            if (cur_index == 0) return false;
            --cur_index;
        } while (value_ids_[cur_index] == pli_sizes_[cur_index] - 1);
        ++value_ids_[cur_index];
        for (++cur_index; cur_index < plis_->size(); ++cur_index) {
            value_ids_[cur_index] = 0;
        }
    }
    return true;
}

void iter::GetCluster() {
    using IterType = std::vector<size_t>::const_iterator;
    std::vector<std::pair<IterType, IterType>> iters;
    iters.reserve(pli_num_);
    size_t min_size = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i < pli_num_; ++i) {
        std::vector<size_t> const& cur_cluster = (*plis_)[i]->GetClusters()[value_ids_[i]];
        iters.emplace_back(cur_cluster.begin(), cur_cluster.end());
        size_t const cur_cluster_size = cur_cluster.size();
        if (cur_cluster_size < min_size) min_size = cur_cluster_size;
    }

    intersection_.reserve(min_size);
    intersection_.clear();
    util::IntersectSortedSequences([this](size_t rec) { intersection_.push_back(rec); }, iters);
}

}
