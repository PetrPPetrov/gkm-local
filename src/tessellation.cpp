// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include <cstdint>
#include <limits>
#include <unordered_map>
#include <thread>
#include <iterator>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "fnv_hash.h"
#include "main.h"
#include "world.h"
#include "request_queue.h"
#include "tessellation.h"

const static boost::posix_time::milliseconds TESSELLATION_WAKE_UP_TIME_INTERVAL(250);
constexpr std::uint32_t TESSELLATION_REQUEST_BUFFER_SIZE = WORLD_BLOCK_SIZE_X * WORLD_BLOCK_HEIGHT * WORLD_BLOCK_HEIGHT;

template <std::uint8_t Level>
static std::unique_ptr<std::thread>& getTessellationThread() {
    static std::unique_ptr<std::thread> tessellation_thread;
    return tessellation_thread;
}

template <std::uint8_t Level>
using TessellationRequestsQueue = RequestQueue<TessellationRequest<Level>, TESSELLATION_REQUEST_BUFFER_SIZE>;

template <std::uint8_t Level>
static TessellationRequestsQueue<Level>& getTessellationRequestQueue() {
    static TessellationRequestsQueue<Level> tessellation_request_queue;
    return tessellation_request_queue;
}

template <std::uint8_t Level>
static std::unique_ptr<boost::asio::io_service>& getIOService() {
    static std::unique_ptr<boost::asio::io_service> io_service;
    return io_service;
}

template <std::uint8_t Level>
static std::unique_ptr<boost::asio::deadline_timer>& getDeadlineTimer() {
    static std::unique_ptr<boost::asio::deadline_timer> timer;
    return timer;
}

template <GlobalCoordinateType Size>
void addSimpleCube(BgfxVertex* vbo, unsigned& vbo_index, GlobalCoordinateType base_x, GlobalCoordinateType base_y, GlobalCoordinateType base_z) {
    constexpr static GlobalCoordinateType SIZE = Size;
    constexpr static GlobalCoordinateType coords[8][3] = {
        { 0, 0, 0 },
        { 1, 0, 0 },
        { 1, 1, 0 },
        { 0, 1, 0 },
        { 0, 0, 1 },
        { 1, 0, 1 },
        { 1, 1, 1 },
        { 0, 1, 1 }
    };
    constexpr static std::int16_t indices[12][5] = {
        { 0, 3, 1,  1, 2 },
        { 3, 2, 1,  1, 2 },
        { 4, 5, 7,  1, 2 },
        { 7, 5, 6,  1, 2 },
        { 0, 1, 4,  1, 3 },
        { 4, 1, 5,  1, 3 },
        { 1, 2, 5,  2, 3 },
        { 5, 2, 6,  2, 3 },
        { 2, 3, 6, -1, 3 },
        { 3, 7, 6, -1, 3 },
        { 3, 0, 4, -2, 3 },
        { 3, 4, 7, -2, 3 }
    };
    constexpr static std::int16_t TEX_COORD_SIZE = static_cast<std::int16_t>(SIZE * TEX_COORD_RATIO);

    const std::int16_t offsets[3] = {
        (base_x % StandardBlock::SIZE) * TEX_COORD_RATIO,
        (base_y % StandardBlock::SIZE) * TEX_COORD_RATIO,
        (base_z % StandardBlock::SIZE) * TEX_COORD_RATIO,
    };

    for (unsigned i = 0; i < std::size(indices); ++i) {
        std::uint16_t u_index = std::abs(indices[i][3]) - 1;
        std::uint16_t v_index = std::abs(indices[i][4]) - 1;
        for (unsigned j = 0; j < 3; ++j) {
            vbo[vbo_index].x = static_cast<float>(base_x + coords[indices[i][j]][0] * SIZE);
            vbo[vbo_index].y = static_cast<float>(base_y + coords[indices[i][j]][1] * SIZE);
            vbo[vbo_index].z = static_cast<float>(base_z + coords[indices[i][j]][2] * SIZE);
            std::int16_t u = coords[indices[i][j]][u_index] * TEX_COORD_SIZE;
            std::int16_t v = coords[indices[i][j]][v_index] * TEX_COORD_SIZE;
            if (indices[i][3] > 0) {
                u += offsets[u_index];
            } else {
                u = TEXTURE_SIZE - u - offsets[u_index];
            }
            if (indices[i][4] > 0) {
                v += offsets[v_index];
            } else {
                v = TEXTURE_SIZE - v - offsets[v_index];
            }
            vbo[vbo_index].u = u;
            vbo[vbo_index].v = v;
            vbo[vbo_index].offset_u_mode = indices[i][3];
            vbo[vbo_index].offset_v_mode = indices[i][4];
            ++vbo_index;
        }
    }
}

template <std::uint16_t Level>
void tessellateBySimpleCube(const typename Block<Level>::Ptr block) {
    const bgfx::Memory* vertex_buffer = bgfx::alloc(static_cast<std::uint32_t>(sizeof(BgfxVertex) * 6 * 2 * 3));
    auto vbo = reinterpret_cast<BgfxVertex*>(vertex_buffer->data);
    unsigned vbo_index = 0;

    addSimpleCube<Block<Level>::SIZE>(vbo, vbo_index, 0, 0, 0);

    MaterialDrawInfo entire_material_draw_info;
    entire_material_draw_info.material = block->material;
    entire_material_draw_info.vertex_buffer = makeBgfxSharedPtr(bgfx::createVertexBuffer(vertex_buffer, BgfxVertex::ms_layout));
    auto block_draw_info = std::make_shared<BlockDrawInfo>();
    block_draw_info->commands.push_back(std::move(entire_material_draw_info));
    block->draw_info.write(block_draw_info);
}

template <std::uint8_t Level>
void tessellateBlock(const typename Block<Level>::Ptr block) {
    std::uint32_t count_per_material[MATERIAL_MAX] = { 0 };
    for (unsigned z = 0; z < Block<Level>::SIZE; ++z) {
        for (unsigned y = 0; y < Block<Level>::SIZE; ++y) {
            for (unsigned x = 0; x < Block<Level>::SIZE; ++x) {
                BlockMaterialType cur_material = block->getMaterial(x, y, z);
                if (cur_material != 0) {
                    ++count_per_material[cur_material];
                }
            }
        }
    }

    const bgfx::Memory* vertex_buffers[MATERIAL_MAX] = { nullptr };
    BgfxVertex* vbos[MATERIAL_MAX] = { nullptr };
    for (unsigned cur_material = 1; cur_material < MATERIAL_MAX; ++cur_material) {
        const auto cur_block_count = count_per_material[cur_material];
        if (cur_block_count > 0) {
            const std::uint32_t triangle_count = cur_block_count * 12;
            const std::uint32_t vertex_count = triangle_count * 3;
            const bgfx::Memory* vertex_buffer = bgfx::alloc(static_cast<std::uint32_t>(sizeof(BgfxVertex) * vertex_count));
            vertex_buffers[cur_material] = vertex_buffer;
            vbos[cur_material] = reinterpret_cast<BgfxVertex*>(vertex_buffer->data);
        }
    }

    unsigned vbo_indices[MATERIAL_MAX] = { 0 };
    for (unsigned z = 0; z < Block<Level>::SIZE; ++z) {
        for (unsigned y = 0; y < Block<Level>::SIZE; ++y) {
            for (unsigned x = 0; x < Block<Level>::SIZE; ++x) {
                BlockMaterialType cur_material = block->getMaterial(x, y, z);
                if (cur_material != 0) {
                    addSimpleCube<1>(vbos[cur_material], vbo_indices[cur_material], x, y, z);
                }
            }
        }
    }

    auto block_draw_info = std::make_shared<BlockDrawInfo>();
    for (unsigned cur_material = 1; cur_material < MATERIAL_MAX; ++cur_material) {
        if (count_per_material[cur_material] > 0) {
            MaterialDrawInfo material_draw_info;
            material_draw_info.material = static_cast<BlockMaterialType>(cur_material);
            material_draw_info.vertex_buffer = makeBgfxSharedPtr(bgfx::createVertexBuffer(vertex_buffers[cur_material], BgfxVertex::ms_layout));
            block_draw_info->commands.push_back(std::move(material_draw_info));
        }
    }
    block->draw_info.write(block_draw_info);
}

template <std::uint8_t Level>
void processTessellationRequest(const TessellationRequest<Level>& request) {
    BlockMaterialType material = request.block->material;
    auto& block = request.block;
    if (!block->draw_info.read()) {
        if (block->entire) {
            if (block->material == 0) {
                // Do nothing, special case when the current block is empty
            } else {
                tessellateBySimpleCube<Level>(block);
            }
        } else {
            if constexpr (Level > 0) {
                for (std::int32_t i = 0; i < Block<Level>::CHILDREN_COUNT; ++i) {
                    TessellationRequest<Level - 1> new_request;
                    new_request.block = block->children[i].read();
                    postBlockTessellationRequest(new_request);
                }
            }
            if constexpr (Level <= 1) {
                tessellateBlock<Level>(block);
            }
        }
    }
}

template <std::uint8_t Level>
static void processTessellationRequests(const boost::system::error_code& error) {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

    TessellationRequest<Level> request;
    while (getTessellationRequestQueue<Level>().peek(request) && g_is_running) {
        processTessellationRequest<Level>(request);
        getTessellationRequestQueue<Level>().pop();
    }
    if (g_is_running) {
        getDeadlineTimer<Level>()->expires_at(getDeadlineTimer<Level>()->expires_at() + TESSELLATION_WAKE_UP_TIME_INTERVAL);
        getDeadlineTimer<Level>()->async_wait(&processTessellationRequests<Level>);
    } else {
        // Clean up tessellation request queue
        TessellationRequest<Level> request;
        while (getTessellationRequestQueue<Level>().peek(request)) {
            getTessellationRequestQueue<Level>().pop();
        }
    }

    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
}

template <std::uint8_t Level>
static void tessellationThread() {
    getIOService<Level>() = std::make_unique<boost::asio::io_service>();
    getDeadlineTimer<Level>() = std::make_unique<boost::asio::deadline_timer>(*getIOService<Level>(), TESSELLATION_WAKE_UP_TIME_INTERVAL);
    getDeadlineTimer<Level>()->async_wait(&processTessellationRequests<Level>);
    getIOService<Level>()->run();
}

template <std::uint8_t Level>
struct TessellationThreadInitializer {
    static void execute() {
        getTessellationThread<Level>() = std::make_unique<std::thread>(&tessellationThread<Level>);
    }
};

void startTessellationThreads() {
    executeForAllLevels<TessellationThreadInitializer>();
}

template <std::uint8_t Level>
struct TessellationThreadWaiter {
    static void execute() {
        getTessellationThread<Level>()->join();
    }
};

void finishTessellationThreads() {
    executeForAllLevels<TessellationThreadWaiter>();
}

template <std::uint8_t Level>
void postBlockTessellationRequest(const TessellationRequest<Level>& request) {
    if (!request.block->tessellation_request) {
        request.block->tessellation_request = true;
        getTessellationRequestQueue<Level>().push(request);
    }
}

template <std::uint8_t Level>
struct PostBlockTessellationRequstInstantiation;

template <>
struct PostBlockTessellationRequstInstantiation<0> {
    static void instantiate() {
        postBlockTessellationRequest<0>(TessellationRequest<0>());
    }
};

template <std::uint8_t Level>
struct PostBlockTessellationRequstInstantiation {
    static void instantiate() {
        postBlockTessellationRequest<Level>(TessellationRequest<Level>());
        PostBlockTessellationRequstInstantiation<Level - 1>::instantiate();
    }
};

template struct PostBlockTessellationRequstInstantiation<TOP_LEVEL>;
