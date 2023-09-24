#include "algorithms/md/hymd/model/dictionary_compressor/dictionary_compressor.h"

#include <numeric>

#include <easylogging++.h>

namespace algos::hymd::model {

DictionaryCompressor::DictionaryCompressor(size_t attribute_num) : plis_(attribute_num) {}

void DictionaryCompressor::AddRecord(std::vector<std::string> record) {
    if (record.size() < plis_.size()) {
        LOG(WARNING) << "Unexpected number of columns for a record, skipping (expected "
                     << plis_.size() << ", got " << record.size()
                     << "). Records processed so far: " << records_processed_ << ".";
        return;
    }
    ++records_processed_;
    CompressedRecord rec(plis_.size());
    for (size_t i = 0; i < plis_.size(); ++i) {
        rec[i] = plis_[i].AddNextValue(record[i]);
    }
    records_.push_back(std::move(rec));
}

std::unique_ptr<DictionaryCompressor> DictionaryCompressor::CreateFrom(
        ::model::IDatasetStream& stream /*, indices*/) {
    size_t const columns = stream.GetNumberOfColumns();
    auto compressor = std::make_unique<DictionaryCompressor>(columns);
    if (!stream.HasNextRow())
        throw std::runtime_error("MD mining is meaningless on empty dataset!");
    while (stream.HasNextRow()) {
        compressor->AddRecord(stream.GetNextRow());
    }
    return compressor;
}

}  // namespace algos::hymd::model
