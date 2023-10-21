// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <vector>
#include "gkm_vec.h"

namespace Gkm {
    struct Ray {
        Vec3 begin;
        Vec3 dir;
    };

    struct RaySegment {
        Ray ray;
        double t0, t1;

        RaySegment() : t0(0), t1(1) {
        }
    };
}

template <std::uint8_t Level>
void calculateIntersectedBlocks(const Gkm::RaySegment& ray, BlockCoords& result);
