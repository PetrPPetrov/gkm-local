// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <memory>
#include <limits>
#include "bgfx_api.h"
#include "game_logic.h"

typedef std::uint8_t BlockMaterialType;

constexpr unsigned MATERIAL_MAX = std::numeric_limits<BlockMaterialType>::max() + 1;

struct DrawRefInfo {
    typedef std::shared_ptr<DrawRefInfo> Ptr;

    BgfxProgramPtr program;
    BgfxUniformPtr texture;
    BgfxTexturePtr material_textures[MATERIAL_MAX];
};

struct DrawInfo {
    std::uint64_t state = BGFX_STATE_DEFAULT;
    DrawRefInfo* ref_info;
};

struct DrawInstanceInfo {
    GlobalCoordinateType x, y, z;
};

struct MaterialDrawInfo {
    BlockMaterialType material;
    BgfxVertexBufferPtr vertex_buffer;
};

struct BlockDrawInfo {
    typedef std::shared_ptr<BlockDrawInfo> Ptr;

    std::list<MaterialDrawInfo> commands;
};
