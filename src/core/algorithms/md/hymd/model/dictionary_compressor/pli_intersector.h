#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/md/hymd/model/dictionary_compressor/keyed_position_list_index.h"
#include "algorithms/md/hymd/types.h"

namespace algos::hymd::model {

// TODO: rework, this should be based on array multimaps of records with values
class PliIntersector {
public:
    struct const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<std::vector<size_t>, std::vector<size_t>>;
        using pointer = value_type*;
        using reference = value_type&;

    private:
        bool is_end_ = false;
        std::vector<std::vector<PliCluster> const*> const* const cluster_indexes_ = {};
        size_t const pli_num_ = {};
        std::vector<size_t> const pli_sizes_ = {};
        std::vector<size_t> value_ids_;
        std::vector<size_t> intersection_;
        using IterType = std::vector<size_t>::const_iterator;
        std::vector<std::pair<IterType, IterType>> iters_;

        void GetCluster();
        bool ValueIdsAreValid();
        bool IncValueIds();

        const_iterator(std::vector<std::vector<PliCluster> const*> const* const cluster_collections,
                       std::vector<size_t> value_ids);
        const_iterator();

    public:
        friend class PliIntersector;

        const_iterator& operator++();
        value_type operator*();

        friend bool operator==(const_iterator const& a, const_iterator const& b);
        friend bool operator!=(const_iterator const& a, const_iterator const& b);
    };

private:
    std::vector<std::vector<PliCluster> const*> const cluster_collections_;

public:
    explicit PliIntersector(std::vector<std::vector<PliCluster> const*> plis);

    [[nodiscard]] const_iterator begin() const;
    [[nodiscard]] const_iterator end() const;
};

}  // namespace algos::hymd::model
