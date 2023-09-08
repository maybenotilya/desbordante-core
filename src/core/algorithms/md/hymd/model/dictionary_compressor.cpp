#include "dictionary_compressor.h"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace {
std::vector<size_t> GetNormalizedIndices(std::vector<size_t> indices) {
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}
}  // namespace

namespace algos::hymd::model {

using CompressedRecord = DictionaryCompressor::CompressedRecord;

DictionaryCompressor::DictionaryCompressor(std::vector<size_t> attribute_indices)
    : indices_(GetNormalizedIndices(std::move(attribute_indices))), plis_(indices_.size()) {}

void DictionaryCompressor::AddRecord(std::vector<std::string> record) {
    if (record.size() < indices_[indices_.size() - 1]) {
        return;
    }
    CompressedRecord rec(indices_.size());
    for (size_t i = 0; i < indices_.size(); ++i) {
        rec[i] = plis_[i].AddNextValue(record[indices_[i]]);
    }
    records_.push_back(std::move(rec));
}

DictionaryCompressor DictionaryCompressor::CreateFrom(
        ::model::IDatasetStream& stream /*, indices*/) {
    size_t const columns = stream.GetNumberOfColumns();
    std::vector<size_t> indices(columns);
    std::iota(indices.begin(), indices.end(), 0);
    DictionaryCompressor compressor{std::move(indices)};
    while (stream.HasNextRow()) {
        compressor.AddRecord(stream.GetNextRow());
    }
    return compressor;
}

}  // namespace algos::hymd::model
