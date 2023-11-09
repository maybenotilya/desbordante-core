#pragma once

#include <memory>
#include <unordered_set>

#include "algorithms/md/hymd/model/dictionary_compressor/keyed_position_list_index.h"
#include "algorithms/md/hymd/types.h"
#include "model/index.h"
#include "model/types/type.h"

namespace algos::hymd::model {

class DataInfo {
private:
    std::unique_ptr<std::byte[]> const data_;
    size_t const elements_;
    size_t const type_size_;
    std::unordered_set<ValueIdentifier> const nulls_;
    std::unordered_set<ValueIdentifier> const empty_;

public:
    DataInfo(std::unique_ptr<std::byte[]> data, size_t elements, size_t type_size,
             std::unordered_set<ValueIdentifier> nulls, std::unordered_set<ValueIdentifier> empty);

    std::byte const* GetAt(::model::Index index) const {
        return data_.get() + type_size_ * index;
    }

    size_t GetElementNumber() const {
        return elements_;
    }

    std::unordered_set<ValueIdentifier> const& GetNulls() const {
        return nulls_;
    }

    std::unordered_set<ValueIdentifier> const& GetEmpty() const {
        return empty_;
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
