#pragma once

#include <memory>
#include <unordered_set>

#include "algorithms/md/hymd/model/dictionary_compressor/keyed_position_list_index.h"
#include "algorithms/md/hymd/types.h"
#include "model/types/type.h"

namespace algos::hymd::model {

struct DataInfo {
    std::unique_ptr<std::byte[]> const data_;
    size_t elements_;
    size_t type_size_;
    std::unordered_set<ValueIdentifier> const nulls_;
    std::unordered_set<ValueIdentifier> const empty_;

public:
    DataInfo(std::unique_ptr<std::byte[]> data, size_t elements, size_t type_size,
             std::unordered_set<ValueIdentifier> nulls, std::unordered_set<ValueIdentifier> empty);

    std::byte const* GetAt(size_t index) const {
        return data_.get() + type_size_ * index;
    }

    size_t GetElementNumber() const {
        return elements_;
    }

    static std::shared_ptr<DataInfo> MakeFrom(KeyedPositionListIndex const& pli,
                                              ::model::Type const& type);
};

template <typename T>
struct ValueInfo {
    std::vector<T> const data;
    std::unordered_set<ValueIdentifier> const nulls;
    std::unordered_set<ValueIdentifier> const empty;

    ValueInfo(std::vector<T> data, std::unordered_set<ValueIdentifier> nulls,
              std::unordered_set<ValueIdentifier> empty)
        : data(std::move(data)), nulls(std::move(nulls)), empty(std::move(empty)) {}
};

}
