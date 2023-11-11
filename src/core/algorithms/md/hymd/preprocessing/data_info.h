#pragma once

#include <cstddef>
#include <memory>
#include <unordered_set>

#include "algorithms/md/hymd/indexes/keyed_position_list_index.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "model/index.h"
#include "model/types/type.h"

namespace algos::hymd::preprocessing {

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

    std::byte const* GetAt(model::Index index) const {
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

    static std::shared_ptr<DataInfo> MakeFrom(indexes::KeyedPositionListIndex const& pli,
                                              model::Type const& type);
};

}  // namespace algos::hymd::preprocessing
