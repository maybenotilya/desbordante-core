#pragma once

#include "algorithms/md/hymd/model/similarity.h"

namespace algos::hymd::model {

class SupportLattice {
public:
    void MarkUnsupported(model::SimilarityVector const& lhs);
    bool IsUnsupported(model::SimilarityVector const& sim_vec);

    SupportLattice(size_t attributes_number);
};

}