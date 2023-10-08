// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include "bgfx_api.h"
#include "vs_pos_tex.h"
#include "fs_unlit.h"

static inline BgfxProgramPtr loadProgram() {
    auto vsh = makeBgfxUniquePtr(bgfx::createShader(bgfx::makeRef(vs_pos_tex, sizeof(vs_pos_tex))));
    auto fsh = makeBgfxUniquePtr(bgfx::createShader(bgfx::makeRef(fs_unlit, sizeof(fs_unlit))));
    return makeBgfxSharedPtr(bgfx::createProgram(*vsh, *fsh));
}
