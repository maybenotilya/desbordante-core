#pragma once

#include <cassert>
#include <cstddef>
#include <map>
#include <optional>
#include <vector>

#include "algorithms/md/hymd/md_lhs.h"
#include "model/index.h"
#include "util/excl_optional.h"

namespace algos::hymd::lattice {

template <typename NodeType>
class NodeBase {
private:
    template <typename T>
    inline static bool NotEmpty(T const& bm) {
        return !bm.empty();
    }

public:
    using BoundMap = std::map<model::md::DecisionBoundary, NodeType>;
    using OptionalChild = util::ExclOptional<BoundMap, NotEmpty<BoundMap>>;
    using Children = std::vector<OptionalChild>;

    Children children;

    NodeBase(std::size_t children_number) : children(children_number) {}

    std::size_t GetChildArraySize(model::Index child_array_index) const noexcept {
        return children.size() - (child_array_index + 1);
    }

    void ForEachNonEmpty(auto action) {
        ForEachNonEmpty(*this, action);
    }

    void ForEachNonEmpty(auto action) const {
        ForEachNonEmpty(*this, action);
    }

    template <typename Self>
    static void ForEachNonEmpty(Self& self, auto action) {
        model::Index index = 0;
        for (std::size_t const array_size = self.children.size(); index != array_size; ++index) {
            auto& optional_child = self.children[index];
            if (optional_child.HasValue()) action(*optional_child, index);
        };
    }

    bool IsEmpty() const noexcept {
        return std::none_of(children.begin(), children.end(),
                            std::mem_fn(&OptionalChild::HasValue));
    }

protected:
    template <typename... Args>
    NodeType* AddOneUncheckedBase(model::Index child_array_index, model::md::DecisionBoundary bound,
                                  Args... args) {
        return &children[child_array_index]
                        ->try_emplace(bound, args..., GetChildArraySize(child_array_index))
                        .first->second;
    }
};

template <typename NodeType>
void AddUnchecked(NodeType* cur_node_ptr, MdLhs const& lhs, MdLhs::iterator cur_lhs_iter,
                  auto final_node_action) {
    assert(cur_node_ptr->IsEmpty());
    for (auto lhs_end = lhs.end(); cur_lhs_iter != lhs_end; ++cur_lhs_iter) {
        auto const& [child_array_index, next_bound] = *cur_lhs_iter;
        cur_node_ptr = cur_node_ptr->AddOneUnchecked(child_array_index, next_bound);
    }
    final_node_action(cur_node_ptr);
}

template <typename NodeType>
void CheckedAdd(NodeType* cur_node_ptr, MdLhs const& lhs, auto const& info, auto unchecked_add,
                auto final_node_action) {
    for (auto lhs_iter = lhs.begin(), lhs_end = lhs.end(); lhs_iter != lhs_end;) {
        auto const& [child_array_index, next_bound] = *lhs_iter;
        ++lhs_iter;
        std::size_t const next_child_array_size =
                cur_node_ptr->GetChildArraySize(child_array_index);
        auto& boundary_map = cur_node_ptr->children[child_array_index];
        if (!boundary_map.HasValue()) {
            NodeType& new_node =
                    boundary_map->try_emplace(next_bound, next_child_array_size).first->second;
            unchecked_add(new_node, info, lhs_iter);
            return;
        }
        auto [it_map, is_first_map] = boundary_map->try_emplace(next_bound, next_child_array_size);
        NodeType& next_node = it_map->second;
        if (is_first_map) {
            unchecked_add(next_node, info, lhs_iter);
            return;
        }
        cur_node_ptr = &next_node;
    };
    final_node_action(cur_node_ptr);
}
}  // namespace algos::hymd::lattice
