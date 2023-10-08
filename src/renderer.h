// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include "win_api.h"
#include "bgfx_api.h"
#include "shader.h"
#include "draw_info.h"
#include "texture_cache.h"
#include "world.h"
#include "flat_terrain.h"

void initializeRenderer(HWND hwnd);
void shutdownRenderer();
