#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_measure.h"

#include <cstddef>

#include "model/types/string_type.h"
#include "util/levenshtein_distance.h"

namespace algos::hymd::preprocessing::similarity_measure {

Similarity LevenshteinMeasure::CalculateSimilarity(std::byte const* value_left,
                                                   std::byte const* value_right) const {
    thread_local std::size_t buf_size = 8;
    thread_local auto buf = std::unique_ptr<unsigned[]>(new unsigned[buf_size * 2]);
    thread_local auto buf1 = buf.get();
    thread_local auto buf2 = buf.get() + buf_size;
    auto const& left = model::Type::GetValue<std::string>(value_left);
    auto const& right = model::Type::GetValue<std::string>(value_right);
    std::size_t buf_required = right.size() + 1;
    if (buf_required > buf_size) {
        buf_size = buf_required * 2;
        buf.reset(new unsigned[buf_size * 2]);
        auto* start = buf.get();
        buf1 = start;
        buf2 = start + buf_size;
    }
    std::size_t const max_dist = std::max(left.size(), right.size());
    Similarity value =
            static_cast<Similarity>(max_dist - util::LevenshteinDistance(left, right, buf1, buf2)) /
            static_cast<Similarity>(max_dist);
    return value;
}

LevenshteinMeasure::LevenshteinMeasure(model::md::DecisionBoundary min_sim)
    : ImmediateSimilarityMeasure("levenshtein", std::make_unique<model::StringType>(), min_sim) {}

}  // namespace algos::hymd::preprocessing::similarity_measure
