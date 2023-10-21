#include "algorithms/md/hymd/model/dictionary_compressor/pli_intersector.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include "util/intersect_sorted_sequences.h"

namespace {
using algos::hymd::model::KeyedPositionListIndex;
using algos::hymd::PliCluster;
std::vector<size_t> GetPliSizes(std::vector<std::vector<PliCluster> const*> const& cluster_collections) {
    std::vector<size_t> pli_sizes;
    pli_sizes.reserve(cluster_collections.size());
    for (auto const* collection_ptr : cluster_collections) {
        pli_sizes.push_back(collection_ptr->size());
    }
    return pli_sizes;
}
}

namespace algos::hymd::model {

using iter = PliIntersector::const_iterator;

PliIntersector::PliIntersector(std::vector<std::vector<PliCluster> const*> cluster_collections)
    : cluster_collections_(std::move(cluster_collections)) {}

iter::value_type iter::operator*() {
    return {value_ids_, intersection_};
}

bool operator!=(iter const& a, iter const& b) {
    return !(a == b);
}

bool operator==(iter const& a, iter const& b) {
    assert(b.is_end_);
    return a.is_end_ == b.is_end_;
}

iter::const_iterator(std::vector<std::vector<PliCluster> const*> const* const cluster_collections,
                     std::vector<size_t> value_ids)
    : cluster_indexes_(cluster_collections),
      pli_num_(cluster_indexes_->size()),
      pli_sizes_(GetPliSizes(*cluster_indexes_)),
      value_ids_(std::move(value_ids)),
      iters_(pli_num_) {
    assert(!cluster_indexes_->empty());
    assert(ValueIdsAreValid());
    GetCluster();
    if (intersection_.empty()) ++*this;
}

iter::const_iterator() : is_end_(true) {}

bool iter::ValueIdsAreValid() {
    for (size_t i = 0; i < pli_num_; ++i) {
        if (pli_sizes_[i] <= value_ids_[i]) return false;
    }
    return true;
}

iter PliIntersector::end() const {
    return const_iterator{};
}

iter PliIntersector::begin() const {
    return {&cluster_collections_, std::vector<size_t>(cluster_collections_.size(), 0)};
}

iter& iter::operator++() {
    while (IncValueIds()) {
        GetCluster();
        if (!intersection_.empty()) return *this;
    }
    is_end_ = true;
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
        for (++cur_index; cur_index < cluster_indexes_->size(); ++cur_index) {
            value_ids_[cur_index] = 0;
        }
    }
    return true;
}

void iter::GetCluster() {
    size_t min_size = std::numeric_limits<size_t>::max();
    assert(pli_num_ != 0);
    assert(iters_.size() == pli_num_);
    for (size_t i = 0; i < pli_num_; ++i) {
        PliCluster const& cur_cluster = cluster_indexes_->operator[](i)->operator[](value_ids_[i]);
        auto& [start_iter, end_iter] = iters_[i];
        start_iter = cur_cluster.begin();
        end_iter = cur_cluster.end();
        size_t const cur_cluster_size = cur_cluster.size();
        if (cur_cluster_size < min_size) min_size = cur_cluster_size;
    }

    intersection_.clear();
    util::IntersectSortedSequences([this](size_t rec) { intersection_.push_back(rec); }, iters_);
}

}
