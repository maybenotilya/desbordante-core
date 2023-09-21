#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

namespace util {

template <typename ReceiverFunc, typename IteratorType>
void IntersectSortedSequences(ReceiverFunc receive, std::vector<IteratorType> iterators,
                              std::vector<IteratorType> const& ends) {
    if (iterators.empty()) return;
    size_t const iterator_num = iterators.size();
    assert(iterator_num == ends.size());
    auto& first_iterator = iterators[0];
    auto& first_end_iterator = ends[0];
    while (first_iterator != first_end_iterator) {
        auto value = *first_iterator;
        size_t cur_index = 1;
        while (cur_index != iterator_num) {
            auto& current_iterator = iterators[cur_index];
            auto const& current_end_iterator = ends[cur_index];
            if (current_iterator == current_end_iterator) return;
            while (*current_iterator < value) {
                ++current_iterator;
                if (current_iterator == current_end_iterator) return;
            }
            if (*current_iterator != value) {
                value = *current_iterator;
                cur_index = 0;
                continue;
            }
            ++cur_index;
        }
        receive(*first_iterator);
        for (auto& it : iterators) {
            ++it;
        }
    }
}

}  // namespace util
