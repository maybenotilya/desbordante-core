#pragma once

#include <cassert>
#include <vector>

#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/decision_boundary_vector.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/lattice/support_lattice.h"
#include "model/index.h"

namespace algos::hymd::lattice {

class FullLattice {
private:
    MdLattice md_lattice_;
    SupportLattice support_lattice_;

    void AddIfMinimal(DecisionBoundaryVector const& lhs_sims,
                      model::md::DecisionBoundary const rhs_sim, model::Index const rhs_index) {
        assert(!support_lattice_.IsUnsupported(lhs_sims));
        md_lattice_.AddIfMinimal(lhs_sims, rhs_sim, rhs_index);
    }

public:
    [[nodiscard]] bool HasGeneralization(DecisionBoundaryVector const& lhs_sims,
                                         model::md::DecisionBoundary const rhs_sim,
                                         model::Index const rhs_index) const {
        return md_lattice_.HasGeneralization(lhs_sims, rhs_sim, rhs_index);
    }

    [[nodiscard]] size_t GetMaxLevel() const {
        return md_lattice_.GetMaxLevel();
    }

    std::vector<MdLatticeNodeInfo> GetLevel(size_t level) {
        // TODO: filter unsupported
        return md_lattice_.GetLevel(level);
    }

    std::vector<model::md::DecisionBoundary> GetMaxValidGeneralizationRhs(
            DecisionBoundaryVector const& lhs) const {
        return md_lattice_.GetMaxValidGeneralizationRhs(lhs);
    }

    void AddIfMinimalAndNotUnsupported(DecisionBoundaryVector const& lhs_sims,
                                       model::md::DecisionBoundary const rhs_sim,
                                       model::Index const rhs_index) {
        if (support_lattice_.IsUnsupported(lhs_sims)) return;
        AddIfMinimal(lhs_sims, rhs_sim, rhs_index);
    }

    std::vector<MdLatticeNodeInfo> FindViolated(SimilarityVector const& similarity_vector) {
        return md_lattice_.FindViolated(similarity_vector);
    }

    void RemoveNode(DecisionBoundaryVector const& lhs) {
        md_lattice_.RemoveNode(lhs);
    }

    void MarkUnsupported(DecisionBoundaryVector const& lhs_vec) {
        support_lattice_.MarkUnsupported(lhs_vec);
    }

    bool IsUnsupported(DecisionBoundaryVector const& lhs_vec) {
        return support_lattice_.IsUnsupported(lhs_vec);
    }

    explicit FullLattice(size_t column_matches_size)
        : md_lattice_(column_matches_size), support_lattice_() {}
};

}  // namespace algos::hymd::lattice
