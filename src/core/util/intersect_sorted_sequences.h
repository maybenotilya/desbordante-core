#pragma once

#include <algorithm>
#include <vector>

namespace util {

template <typename ReceiverFunc, typename IteratorType>
void IntersectSortedSequences(ReceiverFunc receive,
                              std::vector<std::pair<IteratorType, IteratorType>>& iterators) {
    if (iterators.empty()) return;
    size_t const iterator_num = iterators.size();
    std::pair<IteratorType, IteratorType>& first_iter_pair = iterators[0];
    IteratorType& first_iterator = first_iter_pair.first;
    IteratorType const& first_end_iterator = first_iter_pair.second;
    while (first_iterator != first_end_iterator) {
        auto value = *first_iterator;
        size_t cur_index = 1;
        while (cur_index != iterator_num) {
            std::pair<IteratorType, IteratorType>& iter_pair = iterators[cur_index];
            IteratorType& current_iterator = iter_pair.first;
            IteratorType const& current_end_iterator = iter_pair.second;
            if (current_iterator == current_end_iterator) return;
            while (*current_iterator < value) {
                ++current_iterator;
                if (current_iterator == current_end_iterator) return;
            }
            if (*current_iterator != value) {
                value = *current_iterator;
                assert(first_iterator != first_end_iterator);
                ++first_iterator;
                if (first_iterator == first_end_iterator) return;
                while (*first_iterator < value) {
                    ++first_iterator;
                    if (first_iterator == first_end_iterator) return;
                }
                value = *first_iterator;
                cur_index = 1;
                continue;
            }
            ++cur_index;
        }
        for (auto& it : iterators) {
            ++(it.first);
        }
        receive(value);
    }
}

}  // namespace util
