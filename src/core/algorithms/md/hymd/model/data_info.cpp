#include "algorithms/md/hymd/model/data_info.h"

#include <cassert>

namespace algos::hymd::model {

DataInfo::DataInfo(std::unique_ptr<std::byte[]> data, size_t elements, size_t type_size,
                   std::unordered_set<ValueIdentifier> nulls,
                   std::unordered_set<ValueIdentifier> empty)
    : data_(std::move(data)),
      elements_(elements),
      type_size_(type_size),
      nulls_(std::move(nulls)),
      empty_(std::move(empty)) {}

std::shared_ptr<DataInfo> DataInfo::MakeFrom(KeyedPositionListIndex const& pli,
                                             ::model::Type const& type) {
    size_t const value_number = pli.GetClusters().size();
    size_t const type_size = type.GetSize();
    auto data = std::unique_ptr<std::byte[]>(type.Allocate(value_number));
    std::byte* initial = data.get();
    std::unordered_set<ValueIdentifier> nulls;
    std::unordered_set<ValueIdentifier> empty;
    for (auto const& [string, value_id] : pli.GetMapping()) {
        if (string.empty()) {
            empty.insert(value_id);
            continue;
        }
        if (string == ::model::Null::kValue) {
            nulls.insert(value_id);
            continue;
        }
        std::byte* d = initial + value_id * type_size;
        type.ValueFromStr(d, string);
    }
    return std::make_shared<DataInfo>(std::move(data), value_number, type_size, std::move(nulls),
                                      std::move(empty));
}

}
