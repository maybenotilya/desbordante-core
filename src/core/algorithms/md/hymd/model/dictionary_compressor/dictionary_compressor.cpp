#include "dictionary_compressor.h"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace algos::hymd::model {

DictionaryCompressor::DictionaryCompressor(size_t attribute_num) : plis_(attribute_num) {}

void DictionaryCompressor::AddRecord(std::vector<std::string> record) {
    if (record.size() < plis_.size()) {
        return;
    }
    CompressedRecord rec(plis_.size());
    for (size_t i = 0; i < plis_.size(); ++i) {
        rec[i] = plis_[i].AddNextValue(record[i]);
    }
    records_.push_back(std::move(rec));
}

DictionaryCompressor DictionaryCompressor::CreateFrom(
        ::model::IDatasetStream& stream /*, indices*/) {
    size_t const columns = stream.GetNumberOfColumns();
    DictionaryCompressor compressor{columns};
    while (stream.HasNextRow()) {
        compressor.AddRecord(stream.GetNextRow());
    }
    return compressor;
}

}  // namespace algos::hymd::model
