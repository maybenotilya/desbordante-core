#pragma once

#include <string>

#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"

namespace algos::hymd {

class SimilarityMeasureCreator {
private:
    std::string const left_column_name_;
    std::string const right_column_name_;
    std::string const similarity_measure_name_;

public:
    SimilarityMeasureCreator(std::string left_column_name, std::string right_column_name,
                             std::string similarity_measure_name)
        : left_column_name_(std::move(left_column_name)),
          right_column_name_(std::move(right_column_name)),
          similarity_measure_name_(std::move(similarity_measure_name)) {}

    std::string const& GetLeftColumnName() const {
        return left_column_name_;
    }

    std::string const& GetRightColumnName() const {
        return right_column_name_;
    }

    std::string const& GetSimilarityMeasureName() const {
        return similarity_measure_name_;
    }

    virtual std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure> MakeMeasure()
            const = 0;
};

}  // namespace algos::hymd
