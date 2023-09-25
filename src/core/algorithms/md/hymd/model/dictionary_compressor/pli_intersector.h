#include <cstddef>
#include <vector>

#include "algorithms/md/hymd/model/dictionary_compressor/keyed_position_list_index.h"

namespace algos::hymd::model {

class PliIntersector {
public:
    struct const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<std::vector<size_t>, std::vector<size_t>>;
        using pointer = value_type *;
        using reference = value_type &;

    private:
        std::vector<KeyedPositionListIndex const*> const* plis_;
        size_t pli_num_;
        std::vector<size_t> pli_sizes_;
        std::vector<size_t> value_ids_;
        std::vector<size_t> intersection_;

        std::vector<size_t> GetCluster();
        bool ValueIdsAreValid();
        bool IncValueIds();

        const_iterator(std::vector<KeyedPositionListIndex const*> const* plis,
                       std::vector<size_t> value_ids);
        const_iterator(std::vector<KeyedPositionListIndex const*> const* plis);

    public:
        friend class PliIntersector;

        const_iterator& operator++();
        value_type operator*();

        friend bool operator==(const_iterator const& a, const_iterator const& b);
        friend bool operator!=(const_iterator const& a, const_iterator const& b);
    };

private:
    std::vector<KeyedPositionListIndex const*>  plis_;
    const_iterator end_iter_;

public:
    explicit PliIntersector(std::vector<KeyedPositionListIndex const*> plis);

    [[nodiscard]] const_iterator begin() const;
    [[nodiscard]] const_iterator const& end() const;
};

}  // namespace algos::hymd::model