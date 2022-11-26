#include "common_options.h"
#include "pli_based_fd_algorithm.h"

#include <limits>
#include <utility>

namespace algos {

decltype(PliBasedFDAlgorithm::MaxLhsOpt) PliBasedFDAlgorithm::MaxLhsOpt{
        {config::names::kMaximumLhs, config::descriptions::kDMaximumLhs},
        std::numeric_limits<uint>::max()
};

PliBasedFDAlgorithm::PliBasedFDAlgorithm(std::vector<std::string_view> phase_names)
    : FDAlgorithm(std::move(phase_names)) {}

void PliBasedFDAlgorithm::FitInternal(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }
}

std::vector<Column const*> PliBasedFDAlgorithm::GetKeys() const {
    assert(relation_ != nullptr);

    std::vector<Column const*> keys;
    for (ColumnData const& col : relation_->GetColumnData()) {
        if (col.GetPositionListIndex()->GetNumNonSingletonCluster() == 0) {
            keys.push_back(col.GetColumn());
        }
    }

    return keys;
}

void PliBasedFDAlgorithm::Fit(std::shared_ptr<ColumnLayoutRelationData> data) {
    relation_ = std::move(data);
    ExecutePrepare();  // TODO: this method has to be repeated for every "alternative" Fit
}

}  // namespace algos
