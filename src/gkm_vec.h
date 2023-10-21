// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <vector>
#include "gkm_local.h"

namespace Gkm {
    struct Vec3 {
        double x, y, z;

        Vec3() : x(0), y(0), z(0) {
        }

        Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {
        }

        Vec3(const Vec3& a) : x(a.x), y(a.y), z(a.z) {
        }
    };

    Vec3 operator+(const Vec3& a, const Vec3& b) {
        return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    Vec3 operator-(const Vec3& a, const Vec3& b) {
        return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
    }
}

struct BlockCoord {
    BlockIndex x, y, z;

    BlockCoord() : x(0), y(0), z(0) {
    }

    BlockCoord(BlockIndex x_, BlockIndex y_, BlockIndex z_) : x(x_), y(y_), z(z_) {
    }

    BlockCoord(const BlockCoord& coord) : x(coord.x), y(coord.y), z(coord.z) {
    }
};

typedef std::vector<BlockCoord> BlockCoords;
