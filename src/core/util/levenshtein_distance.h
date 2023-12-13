#pragma once

#include <string_view>

namespace util {

unsigned LevenshteinDistance(std::string_view l, std::string_view r);
unsigned LevenshteinDistance(std::string_view l, std::string_view r, unsigned* v0, unsigned* v1);

}  // namespace util
