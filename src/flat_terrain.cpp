// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include "main.h"
#include "constants.sh"
#include "flat_terrain.h"

FlatTerrain::FlatTerrain(const BgfxTexturePtr& texture) {
    //const bgfx::Memory* vertex_buffer_data = bgfx::alloc(static_cast<std::uint32_t>(sizeof(BgfxVertex) * 3));
    //auto vbo = reinterpret_cast<BgfxVertex*>(vertex_buffer_data->data);
    //vbo[0].x = 0.0f;
    //vbo[0].y = 10.0f;
    //vbo[0].z = 0.0f;
    //vbo[1].x = 10.0f;
    //vbo[1].y = 10.0f;
    //vbo[1].z = 0.0f;
    //vbo[2].x = 0.0f;
    //vbo[2].y = 10.0f;
    //vbo[2].z = 10.0f;
    //vbo[0].u = TEX_COORD_MIN;
    //vbo[1].u = TEX_COORD_MAX;
    //vbo[2].u = TEX_COORD_MIN;
    //vbo[0].v = TEX_COORD_MIN;
    //vbo[1].v = TEX_COORD_MIN;
    //vbo[2].v = TEX_COORD_MAX;
    //vertex_buffer = makeBgfxSharedPtr(bgfx::createVertexBuffer(vertex_buffer_data, BgfxVertex::ms_layout));
    //terrain_texture = g_renderer->getTextureCache()->getTexture(0).bgfx_texture;

    //constexpr std::int32_t GRID_SIZE = 1024;
    //constexpr std::int32_t VERTEX_COUNT = GRID_SIZE * GRID_SIZE * 6;
    //constexpr std::int32_t CELL_SIZE = 10;
    //const bgfx::Memory* vertex_buffer_data = bgfx::alloc(static_cast<std::uint32_t>(sizeof(BgfxVertex) * VERTEX_COUNT));
    //auto vbo = reinterpret_cast<BgfxVertex*>(vertex_buffer_data->data);
    //for (std::int32_t y = 0; y < GRID_SIZE; ++y) {
    //    for (std::int32_t x = 0; x < GRID_SIZE; ++x) {
    //        std::int32_t actual_x = (x - GRID_SIZE / 2) * CELL_SIZE;
    //        std::int32_t actual_y = (y - GRID_SIZE / 2) * CELL_SIZE;
    //        std::uint32_t base_index = y * GRID_SIZE + x;
    //        BgfxVertex* cell_data = vbo + base_index * 6;
    //        cell_data[0].x = static_cast<float>(actual_x);
    //        cell_data[0].y = static_cast<float>(actual_y);
    //        cell_data[0].z = 0.0f;
    //        cell_data[1].x = static_cast<float>(actual_x + CELL_SIZE);
    //        cell_data[1].y = static_cast<float>(actual_y);
    //        cell_data[1].z = 0.0f;
    //        cell_data[2].x = static_cast<float>(actual_x);
    //        cell_data[2].y = static_cast<float>(actual_y + CELL_SIZE);
    //        cell_data[2].z = 0.0f;
    //        cell_data[3].x = static_cast<float>(actual_x);
    //        cell_data[3].y = static_cast<float>(actual_y + CELL_SIZE);
    //        cell_data[3].z = 0.0f;
    //        cell_data[4].x = static_cast<float>(actual_x + CELL_SIZE);
    //        cell_data[4].y = static_cast<float>(actual_y);
    //        cell_data[4].z = 0.0f;
    //        cell_data[5].x = static_cast<float>(actual_x + CELL_SIZE);
    //        cell_data[5].y = static_cast<float>(actual_y + CELL_SIZE);
    //        cell_data[5].z = 0.0f;
    //        if (x % 2) {
    //            cell_data[0].u = TEX_COORD_MIN;
    //            cell_data[1].u = TEX_COORD_MAX;
    //            cell_data[2].u = TEX_COORD_MIN;
    //            cell_data[3].u = TEX_COORD_MIN;
    //            cell_data[4].u = TEX_COORD_MAX;
    //            cell_data[5].u = TEX_COORD_MAX;
    //        } else {
    //            cell_data[0].u = TEX_COORD_MAX;
    //            cell_data[1].u = TEX_COORD_MIN;
    //            cell_data[2].u = TEX_COORD_MAX;
    //            cell_data[3].u = TEX_COORD_MAX;
    //            cell_data[4].u = TEX_COORD_MIN;
    //            cell_data[5].u = TEX_COORD_MIN;
    //        }
    //        if (y % 2) {
    //            cell_data[0].v = TEX_COORD_MIN;
    //            cell_data[1].v = TEX_COORD_MIN;
    //            cell_data[2].v = TEX_COORD_MAX;
    //            cell_data[3].v = TEX_COORD_MAX;
    //            cell_data[4].v = TEX_COORD_MIN;
    //            cell_data[5].v = TEX_COORD_MAX;
    //        } else {
    //            cell_data[0].v = TEX_COORD_MAX;
    //            cell_data[1].v = TEX_COORD_MAX;
    //            cell_data[2].v = TEX_COORD_MIN;
    //            cell_data[3].v = TEX_COORD_MIN;
    //            cell_data[4].v = TEX_COORD_MAX;
    //            cell_data[5].v = TEX_COORD_MIN;
    //        }
    //    }
    //}

    constexpr std::int32_t GRID_SIZE = 30;
    constexpr std::int32_t VERTEX_COUNT = GRID_SIZE * GRID_SIZE * 6;
    constexpr std::int32_t CELL_SIZE = 1000;
    const bgfx::Memory* vertex_buffer_data = bgfx::alloc(static_cast<std::uint32_t>(sizeof(BgfxVertex) * VERTEX_COUNT));
    auto vbo = reinterpret_cast<BgfxVertex*>(vertex_buffer_data->data);
    for (std::int32_t y = 0; y < GRID_SIZE; ++y) {
        for (std::int32_t x = 0; x < GRID_SIZE; ++x) {
            std::int32_t actual_x = (x - GRID_SIZE / 2) * CELL_SIZE;
            std::int32_t actual_y = (y - GRID_SIZE / 2) * CELL_SIZE;
            std::uint32_t base_index = y * GRID_SIZE + x;
            BgfxVertex* cell_data = vbo + base_index * 6;
            cell_data[0].x = static_cast<float>(actual_x);
            cell_data[0].y = static_cast<float>(actual_y);
            cell_data[0].z = 0.0f;
            cell_data[1].x = static_cast<float>(actual_x + CELL_SIZE);
            cell_data[1].y = static_cast<float>(actual_y);
            cell_data[1].z = 0.0f;
            cell_data[2].x = static_cast<float>(actual_x);
            cell_data[2].y = static_cast<float>(actual_y + CELL_SIZE);
            cell_data[2].z = 0.0f;
            cell_data[3].x = static_cast<float>(actual_x);
            cell_data[3].y = static_cast<float>(actual_y + CELL_SIZE);
            cell_data[3].z = 0.0f;
            cell_data[4].x = static_cast<float>(actual_x + CELL_SIZE);
            cell_data[4].y = static_cast<float>(actual_y);
            cell_data[4].z = 0.0f;
            cell_data[5].x = static_cast<float>(actual_x + CELL_SIZE);
            cell_data[5].y = static_cast<float>(actual_y + CELL_SIZE);
            cell_data[5].z = 0.0f;
            cell_data[0].u = 0;
            cell_data[1].u = TEXTURE_SIZE;
            cell_data[2].u = 0;
            cell_data[3].u = 0;
            cell_data[4].u = TEXTURE_SIZE;
            cell_data[5].u = TEXTURE_SIZE;
            cell_data[0].v = 0;
            cell_data[1].v = 0;
            cell_data[2].v = TEXTURE_SIZE;
            cell_data[3].v = TEXTURE_SIZE;
            cell_data[4].v = 0;
            cell_data[5].v = TEXTURE_SIZE;
        }
    }

    vertex_buffer = makeBgfxSharedPtr(bgfx::createVertexBuffer(vertex_buffer_data, BgfxVertex::ms_layout));
    terrain_texture = texture;
}

void FlatTerrain::draw(DrawInfo draw_info) {
    float float_matrix[4][4] = { 0 };
    float_matrix[0][0] = 1.0f;
    float_matrix[1][1] = 1.0f;
    float_matrix[2][2] = 1.0f;
    float_matrix[3][3] = 1.0f;
    bgfx::setTransform(float_matrix);

    bgfx::setTexture(0, *draw_info.ref_info->texture, *terrain_texture);
    bgfx::setVertexBuffer(0, *vertex_buffer);
    bgfx::setState(draw_info.state);
    bgfx::submit(0, *draw_info.ref_info->program);
}
