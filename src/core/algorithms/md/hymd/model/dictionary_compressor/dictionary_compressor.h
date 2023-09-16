#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "keyed_position_list_index.h"
#include "model/table/idataset_stream.h"

namespace algos::hymd::model {

class DictionaryCompressor {
public:
    using CompressedRecord = std::vector<size_t>;

private:
    std::vector<size_t> indices_;
    std::vector<KeyedPositionListIndex> plis_;
    std::vector<CompressedRecord> records_;

public:
    explicit DictionaryCompressor(std::vector<size_t> attribute_indices);
    void AddRecord(std::vector<std::string> record);
    [[nodiscard]] std::vector<KeyedPositionListIndex> const& GetPlis() const {
        return plis_;
    };
    [[nodiscard]] std::vector<CompressedRecord> const& GetRecords() const {
        return records_;
    }

    static DictionaryCompressor CreateFrom(::model::IDatasetStream& stream);
};

}  // namespace algos::hymd::model