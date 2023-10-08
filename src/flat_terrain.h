// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include "bgfx_api.h"
#include "shader.h"
#include "draw_info.h"

class FlatTerrain {
public:
    typedef std::unique_ptr<FlatTerrain> Ptr;

    FlatTerrain(const BgfxTexturePtr& texture);
    void draw(DrawInfo draw_info);

private:
    BgfxVertexBufferPtr vertex_buffer;
    BgfxTexturePtr terrain_texture;
};
