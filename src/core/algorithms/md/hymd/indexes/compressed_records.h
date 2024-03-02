#pragma once

#include "algorithms/md/hymd/indexes/dictionary_compressor.h"

namespace algos::hymd::indexes {

class CompressedRecords {
    std::shared_ptr<DictionaryCompressor const> const records_left_;
    std::shared_ptr<DictionaryCompressor const> const records_right_;

public:
    [[nodiscard]] bool OneTableGiven() const noexcept {
        return records_left_ == records_right_;
    }

    [[nodiscard]] DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return *records_left_;
    }

    [[nodiscard]] DictionaryCompressor const& GetRightCompressor() const noexcept {
        return *records_right_;
    }

    CompressedRecords(std::shared_ptr<DictionaryCompressor> records_left,
                      std::shared_ptr<DictionaryCompressor> records_right) noexcept
        : records_left_(std::move(records_left)), records_right_(std::move(records_right)) {}

    static std::unique_ptr<CompressedRecords> CreateFrom(model::IDatasetStream& left_table) {
        std::shared_ptr<DictionaryCompressor> left_compressed =
                DictionaryCompressor::CreateFrom(left_table);
        return std::make_unique<CompressedRecords>(left_compressed, left_compressed);
    }

    static std::unique_ptr<CompressedRecords> CreateFrom(model::IDatasetStream& left_table,
                                                         model::IDatasetStream& right_table) {
        return std::make_unique<CompressedRecords>(DictionaryCompressor::CreateFrom(left_table),
                                                   DictionaryCompressor::CreateFrom(right_table));
    }
};

}  // namespace algos::hymd::indexes
