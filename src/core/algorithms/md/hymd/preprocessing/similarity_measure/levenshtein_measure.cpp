#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_measure.h"

#include <cstddef>

#include "model/types/string_type.h"
#include "util/levenshtein_distance.h"

namespace algos::hymd::preprocessing::similarity_measure {

Similarity LevenshteinMeasure::CalculateSimilarity(std::byte const* value_left,
                                                   std::byte const* value_right) const {
    auto const& left = model::Type::GetValue<std::string>(value_left);
    auto const& right = model::Type::GetValue<std::string>(value_right);
    std::size_t const max_dist = std::max(left.size(), right.size());
    Similarity value = static_cast<Similarity>(max_dist - util::LevenshteinDistance(left, right)) /
                       static_cast<Similarity>(max_dist);
    return value;
}

LevenshteinMeasure::LevenshteinMeasure(model::md::DecisionBoundary min_sim)
    : ImmediateSimilarityMeasure("levenshtein", std::make_unique<model::StringType>(), min_sim) {}

}  // namespace algos::hymd::preprocessing::similarity_measure
