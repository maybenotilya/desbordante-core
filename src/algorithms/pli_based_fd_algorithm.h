#pragma once

#include "column_layout_relation_data.h"
#include "fd_algorithm.h"

namespace algos {

class PliBasedFDAlgorithm : public FDAlgorithm {
protected:
    using MaxLhsType = unsigned int;
    static const config::OptionType<MaxLhsType> MaxLhsOpt;

    std::shared_ptr<ColumnLayoutRelationData> relation_;

    void FitFd(model::IDatasetStream& data_stream) final;

    ColumnLayoutRelationData const& GetRelation() const noexcept {
        // GetRelation should be called after the dataset has been parsed, i.e.
        // after calling Fit
        assert(relation_ != nullptr);
        return *relation_;
    }

public:
    explicit PliBasedFDAlgorithm(std::vector<std::string_view> phase_names);

    std::vector<Column const*> GetKeys() const override;

    using Primitive::Fit;
    void Fit(std::shared_ptr<ColumnLayoutRelationData> data);
};

}  // namespace algos
