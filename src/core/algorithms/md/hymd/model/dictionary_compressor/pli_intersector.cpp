#include "algorithms/md/hymd/model/dictionary_compressor/pli_intersector.h"

#include <algorithm>
#include <cassert>

namespace {
using algos::hymd::model::KeyedPositionListIndex;
std::vector<size_t> GetEndingValueIds(std::vector<KeyedPositionListIndex const*> const& plis) {
    std::vector<size_t> value_ids;
    value_ids.reserve(plis.size());
    for (auto const* pli : plis) {
        value_ids.push_back(pli->GetClusters().size());
    }
    return value_ids;
}
}

namespace algos::hymd::model {

using iter = PliIntersector::const_iterator;

PliIntersector::PliIntersector(std::vector<KeyedPositionListIndex const*> plis)
    : plis_(std::move(plis)) {}

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
    : plis_(plis), value_ids_(std::move(value_ids)), intersection_(GetCluster()) {
    assert(!plis_->empty());
    assert(ValueIdsAreValid());
    if (intersection_.empty()) ++*this;
}

iter::const_iterator(std::vector<KeyedPositionListIndex const*> const* plis)
    : plis_(plis), value_ids_(GetEndingValueIds(*plis)), intersection_() {}

bool iter::ValueIdsAreValid() {
    for (size_t i = 0; i < plis_->size(); ++i) {
        if ((*plis_)[i]->GetClusters().size() > value_ids_[i]) return false;
    }
    return true;
}

iter PliIntersector::end() const {
    return {&plis_};
}

iter PliIntersector::begin() const {
    return {&plis_, std::vector<size_t>(plis_.size(), 0)};
}

iter& iter::operator++() {
    std::vector<size_t> cluster;
    while (IncValueIds()) {
        cluster = GetCluster();
        if (!cluster.empty()) break;
    }
    intersection_ = cluster;
    return *this;
}

bool iter::IncValueIds() {
    assert(ValueIdsAreValid());
    size_t cur_index = plis_->size() - 1;
    ++value_ids_[cur_index];
    if (value_ids_[cur_index] >= (*plis_)[cur_index]->GetClusters().size()) {
        do {
            if (cur_index == 0) return false;
            --cur_index;
        } while (value_ids_[cur_index] == (*plis_)[cur_index]->GetClusters().size() - 1);
        ++value_ids_[cur_index];
        for (++cur_index; cur_index < plis_->size(); ++cur_index) {
            value_ids_[cur_index] = 0;
        }
    }
    return true;
}

std::vector<size_t> iter::GetCluster() {
    std::vector<size_t> cluster = (*plis_)[0]->GetClusters()[value_ids_[0]];
    for (size_t i = 1; i < value_ids_.size(); ++i) {
        std::vector<size_t> new_cluster = (*plis_)[i]->GetClusters()[value_ids_[i]];
        std::vector<size_t> intersected;
        std::set_intersection(cluster.begin(), cluster.end(), new_cluster.begin(),
                              new_cluster.end(), std::back_inserter(intersected));
        cluster.swap(intersected);
    }
    return cluster;
}

}
