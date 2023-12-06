#pragma once

#include <cassert>
#include <future>
#include <system_error>
#include <thread>
#include <vector>

#include <easylogging++.h>

namespace util {

/* Parallel version of std::for_each which allows to specify the number of threads to use.
 * If threads_num_max == 1 then behaves like a sequential std::for_each.
 * NOTE: actual number of threads to be used is minimum of the
 *       std::distance(begin, end) and threads_num_max.
 */
template <typename It, typename UnaryFunction>
inline void ParallelForeach(It begin, It end, unsigned const threads_num_max, UnaryFunction f) {
    assert(threads_num_max != 0);
    auto const length = std::distance(begin, end);
    if (length == 0) {
        return;
    }
    auto const threads_num_actual =
            static_cast<unsigned>(std::min(length, static_cast<decltype(length)>(threads_num_max)));
    auto const items_per_thread = std::distance(begin, end) / threads_num_actual;
    std::vector<std::thread> threads;
    threads.reserve(threads_num_actual);

    auto const task = [&f](It first, It last) {
        for (; first != last; ++first) {
            f(*first);
        }
    };

    It p = begin;
    for (unsigned i = 0; i < threads_num_actual - 1; ++i) {
        It prev = p;
        std::advance(p, items_per_thread);
        try {
            threads.emplace_back(task, prev, p);
        } catch (std::system_error const& e) {
            /* Could not create a new thread */
            LOG(WARNING) << "Created " << threads.size() << " threads in ParallelForeach. "
                         << "Could not create new thread: " << e.what();
            /* Fall through to the serial case for the remaining elements */
            p = prev;
            break;
        }
    }

    for (; p != end; ++p) {
        f(*p);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

template <typename It, typename UnaryFunction>
inline void parallel_foreach_async(It begin, It end, unsigned const tasks_num_max,
                                   UnaryFunction f) {
    assert(threads_num_max != 0);
    auto const length = std::distance(begin, end);
    if (length == 0) {
        return;
    }
    auto const threads_num_actual =
            static_cast<unsigned>(std::min(length, static_cast<decltype(length)>(tasks_num_max)));
    auto const items_per_thread = std::distance(begin, end) / threads_num_actual;
    std::vector<std::future<void>> tasks;
    tasks.reserve(threads_num_actual);

    auto const task = [&f](It first, It last) {
        for (; first != last; ++first) {
            f(*first);
        }
    };

    It p = begin;
    for (unsigned i = 0; i < threads_num_actual - 1; ++i) {
        It prev = p;
        std::advance(p, items_per_thread);
        try {
            tasks.push_back(std::async(task, prev, p));
        } catch (std::system_error const& e) {
            /* Could not create a new thread */
            LOG(WARNING) << "Created " << tasks.size() << " tasks in parallel_foreach_async. "
                         << "Could not create new task: " << e.what();
            /* Fall through to the serial case for the remaining elements */
            p = prev;
            break;
        }
    }

    for (; p != end; ++p) {
        f(*p);
    }

    for (auto& task : tasks) {
        task.get();
    }
}

}  // namespace util
