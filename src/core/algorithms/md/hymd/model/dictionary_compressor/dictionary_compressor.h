#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/model/dictionary_compressor/keyed_position_list_index.h"
#include "model/table/idataset_stream.h"

namespace algos::hymd::model {

class DictionaryCompressor {
public:
    using CompressedRecord = std::vector<size_t>;

private:
    std::vector<KeyedPositionListIndex> plis_;
    std::vector<CompressedRecord> records_;
    size_t records_processed_ = 0;

public:
    explicit DictionaryCompressor(size_t attribute_num);
    void AddRecord(std::vector<std::string> record);
    [[nodiscard]] KeyedPositionListIndex const& GetPli(size_t column_index) const {
        return plis_[column_index];
    };
    [[nodiscard]] std::vector<CompressedRecord> const& GetRecords() const {
        return records_;
    }
    [[nodiscard]] size_t GetNumberOfRecords() const {
        return records_processed_;
    }

    static DictionaryCompressor CreateFrom(::model::IDatasetStream& stream);
};

}  // namespace algos::hymd::model