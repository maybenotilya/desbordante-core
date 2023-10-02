#pragma once

#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"

namespace algos::hymd::model {

class CompressedRecords {
    model::DictionaryCompressor records_left_;
    model::DictionaryCompressor records_right_;

public:
    [[nodiscard]] model::DictionaryCompressor const& GetLeftRecords() const {
        return records_left_;
    }

    [[nodiscard]] model::DictionaryCompressor const& GetRightRecords() const {
        return records_right_;
    }

    CompressedRecords(model::DictionaryCompressor records_left,
                      model::DictionaryCompressor records_right)
        : records_left_(std::move(records_left)), records_right_(std::move(records_right)) {}

    static std::unique_ptr<CompressedRecords> CreateFrom(::model::IDatasetStream& left_table,
                                                         ::model::IDatasetStream& right_table) {
        return std::make_unique<CompressedRecords>(
                model::DictionaryCompressor::CreateFrom(left_table),
                model::DictionaryCompressor::CreateFrom(right_table));
    }
};

}
