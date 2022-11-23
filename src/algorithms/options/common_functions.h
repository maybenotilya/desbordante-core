#pragma once

template <typename T>
auto equals(T const& to) {
    return [to](auto value) { return value == to; };
}

bool true_func(...);
