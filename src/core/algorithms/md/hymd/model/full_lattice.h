#pragma once

#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"
#include "algorithms/md/hymd/model/support_lattice/support_lattice.h"

namespace algos::hymd::model {

class FullLattice {
private:
    MdLattice md_lattice_;
    SupportLattice support_lattice_;

    void AddIfMin(SimilarityVector const& lhs_sims, Similarity rhs_sim, size_t rhs_index) {
        assert(!support_lattice_.IsUnsupported(lhs_sims, rhs_sim, rhs_index));
        md_lattice_.AddIfMin(lhs_sims, rhs_sim, rhs_index);
    }

public:
    [[nodiscard]] bool HasGeneralization(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                         size_t rhs_index) const {
        return md_lattice_.HasGeneralization(lhs_sims, rhs_sim, rhs_index);
    }

    [[nodiscard]] size_t GetMaxLevel() const {
        return md_lattice_.GetMaxLevel();
    }

    std::vector<LatticeNodeSims> GetLevel(size_t level) const {
        return md_lattice_.GetLevel(level);
    }

    SimilarityVector GetMaxValidGeneralizationRhs(SimilarityVector const& lhs) const {
        return md_lattice_.GetMaxValidGeneralizationRhs(lhs);
    }

    void Add(LatticeMd const& md) {
        md_lattice_.Add(md.lhs_sims, md.rhs_sim, md.rhs_index);
    }

    void AddIfMinAndNotUnsupported(SimilarityVector const& lhs_sims, Similarity rhs_sim,
                                   size_t rhs_index) {
        if (support_lattice_.IsUnsupported(lhs_sims)) return;
        AddIfMin(lhs_sims, rhs_sim, rhs_index);
    }

    void AddIfMinAndNotUnsupported(LatticeMd const& md) {
        if (support_lattice_.IsUnsupported(md.lhs_sims)) return;
        AddIfMin(md.lhs_sims, md.rhs_sim, md.rhs_index);
    }

    std::vector<MdLatticeNodeInfo> FindViolated(SimilarityVector const& similarity_vector) {
        return md_lattice_.FindViolated(similarity_vector);
    }

    void RemoveNode(SimilarityVector const& lhs) {
        md_lattice_.RemoveNode(lhs);
    }

    void MarkUnsupported(model::SimilarityVector const& lhs_vec) {
        support_lattice_.MarkUnsupported(lhs_vec);
    }

    bool IsUnsupported(model::SimilarityVector const& lhs_vec) {
        return support_lattice_.IsUnsupported(lhs_vec);
    }

    explicit FullLattice(size_t column_matches_size)
        : md_lattice_(column_matches_size), support_lattice_() {}
};

}
