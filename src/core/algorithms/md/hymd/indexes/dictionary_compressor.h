#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/compressed_record.h"
#include "algorithms/md/hymd/indexes/keyed_position_list_index.h"
#include "model/index.h"
#include "model/table/idataset_stream.h"

namespace algos::hymd::indexes {

class DictionaryCompressor {
private:
    std::vector<KeyedPositionListIndex> plis_;
    std::vector<CompressedRecord> records_;
    std::size_t records_processed_ = 0;

public:
    explicit DictionaryCompressor(std::size_t attribute_num);
    void AddRecord(std::vector<std::string> record);
    [[nodiscard]] KeyedPositionListIndex const& GetPli(model::Index column_index) const {
        return plis_[column_index];
    };
    [[nodiscard]] std::vector<CompressedRecord> const& GetRecords() const {
        return records_;
    }
    [[nodiscard]] std::size_t GetNumberOfRecords() const {
        return records_processed_;
    }

    static std::unique_ptr<DictionaryCompressor> CreateFrom(model::IDatasetStream& stream);
};

}  // namespace algos::hymd::indexes
