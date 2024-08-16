#pragma once

#include <string>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/indexes/records_info.h"
#include "model/table/relational_schema.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd::preprocessing::similarity_measure {

class SimilarityMeasure {
private:
    bool is_symmetrical_and_eq_is_max_;
    std::string name_;

public:
    SimilarityMeasure(bool is_symmetrical_and_eq_is_max, std::string name) noexcept
        : is_symmetrical_and_eq_is_max_(is_symmetrical_and_eq_is_max), name_(std::move(name)) {}

    virtual ~SimilarityMeasure() = default;

    [[nodiscard]] virtual indexes::SimilarityMeasureOutput MakeIndexes(
            util::WorkerThreadPool* pool_ptr, indexes::RecordsInfo const& records_info) const = 0;

    virtual void SetParameters(RelationalSchema const& left_schema,
                               RelationalSchema const& right_schema) = 0;

    virtual std::pair<model::Index, model::Index> GetIndices() const noexcept = 0;

    [[nodiscard]] bool IsSymmetricalAndEqIsMax() const noexcept {
        return is_symmetrical_and_eq_is_max_;
    }

    std::string const& GetName() const noexcept {
        return name_;
    }
};

}  // namespace algos::hymd::preprocessing::similarity_measure
