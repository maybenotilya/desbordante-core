#pragma once

#include "algorithms/md/hymd/model/md_lattice/md_lattice.h"
#include "algorithms/md/hymd/model/support_lattice/support_lattice.h"

namespace algos::hymd::model {

class FullLattice {
private:
    MdLattice md_lattice_;
    SupportLattice support_lattice_;

    void AddIfMin(LatticeMd const& md) {
        assert(!support_lattice_.IsUnsupported(md.lhs_sims));
        md_lattice_.AddIfMin(md);
    }

public:
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
        md_lattice_.Add(md);
    }

    void AddIfMinAndNotUnsupported(LatticeMd const& md) {
        if (support_lattice_.IsUnsupported(md.lhs_sims)) return;
        AddIfMin(md);
    }

    std::vector<LatticeMd> FindViolated(SimilarityVector const& similarity_vector) {
        return md_lattice_.FindViolated(similarity_vector);
    }

    void RemoveMd(LatticeMd const& md) {
        md_lattice_.RemoveMd(md);
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
