#include "algorithms/md/hymd/preprocessing/data_info.h"

#include <cstddef>

namespace algos::hymd::preprocessing {

DataInfo::DataInfo(std::unique_ptr<std::byte[]> data, std::size_t elements, std::size_t type_size,
                   std::unordered_set<ValueIdentifier> nulls,
                   std::unordered_set<ValueIdentifier> empty)
    : data_(std::move(data)),
      elements_(elements),
      type_size_(type_size),
      nulls_(std::move(nulls)),
      empty_(std::move(empty)) {}

std::shared_ptr<DataInfo> DataInfo::MakeFrom(indexes::KeyedPositionListIndex const& pli,
                                             model::Type const& type) {
    std::size_t const value_number = pli.GetClusters().size();
    std::size_t const type_size = type.GetSize();
    auto data = std::unique_ptr<std::byte[]>(type.Allocate(value_number));
    std::unordered_set<ValueIdentifier> nulls;
    std::unordered_set<ValueIdentifier> empty;
    for (auto const& [string, value_id] : pli.GetMapping()) {
        if (string.empty()) {
            empty.insert(value_id);
            continue;
        }
        if (string == model::Null::kValue) {
            nulls.insert(value_id);
            continue;
        }
        type.ValueFromStr(&data[value_id * type_size], string);
    }
    return std::make_shared<DataInfo>(std::move(data), value_number, type_size, std::move(nulls),
                                      std::move(empty));
}

}  // namespace algos::hymd::preprocessing
