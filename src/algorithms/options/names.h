#pragma once

namespace option_names {
constexpr auto kData = "data";
constexpr auto kSeparator = "separator";
constexpr auto kHasHeader = "has_header";
constexpr auto kEqualNulls = "is_null_equal_null";
constexpr auto kThreads = "threads";
constexpr auto kError = "error";
constexpr auto kMaximumLhs = "max_lhs";
constexpr auto kSeed = "seed";
constexpr auto kMinimumSupport = "minsup";
constexpr auto kMinimumConfidence = "minconf";
constexpr auto kInputFormat = "input_format";
constexpr auto kTIdColumnIndex = "tid_column_index";
constexpr auto kItemColumnIndex = "item_column_index";
constexpr auto kFirstColumnTId = "has_tid";
constexpr auto kMetric = "metric";
constexpr auto kLhsIndices = "lhs_indices";
constexpr auto kRhsIndices = "rhs_indices";
constexpr auto kParameter = "parameter";
constexpr auto kDistToNullIsInfinity = "dist_to_null_infinity";
constexpr auto kQGramLength = "q";
constexpr auto kMetricAlgorithm = "metric_algorithm";
constexpr auto kRadius = "radius";
constexpr auto kRatio = "ratio";
}  // namespace option_names