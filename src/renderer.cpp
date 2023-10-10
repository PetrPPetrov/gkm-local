// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include <boost/asio.hpp>
#include "main.h"
#include "user_interface.h"
#include "game_logic.h"
#include "world.h"
#include "tessellation.h"
#include "block_operation.h"
#include "renderer.h"

bgfx::VertexLayout BgfxVertex::ms_layout;

class Renderer {
public:
    typedef std::unique_ptr<Renderer> Ptr;

    Renderer(std::uint32_t width, std::uint32_t height, void* native_windows_handle, bgfx::RendererType::Enum render_type);

    void init();
    TextureCache* getTextureCache() const;

    void render(int window_width, int window_height);

private:
    BgfxEngineShutdown bgfx_engine_shutdown;

    TextureCache::Ptr texture_cache;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t debug;
    std::uint32_t reset;
    DrawRefInfo::Ptr draw_ref_info;
    FlatTerrain::Ptr terrain;
};

static Renderer::Ptr g_renderer = nullptr;
static std::unique_ptr<std::thread> g_render_thread = nullptr;
static std::unique_ptr<boost::asio::io_service> g_io_service;
static std::unique_ptr<boost::asio::deadline_timer> g_timer;
static boost::posix_time::microseconds g_render_interval(12000);

static std::unique_ptr<boost::asio::deadline_timer> g_fps_timer;
const static boost::posix_time::microseconds FPS_COUNTING_INTERVAL(500000);
constexpr static float ONE_SECOND_IN_MC = 1000000.0f;
const static float FPS_MULTIPLIER = ONE_SECOND_IN_MC / FPS_COUNTING_INTERVAL.total_microseconds();
static unsigned g_fps = 0;
constexpr static float TARGET_FPS = 60.0f;
static_assert(TARGET_FPS > 1.0f);

static HWND g_hwnd = 0;

static void render(const boost::system::error_code& error) {
    if (g_is_running) {
        g_timer->expires_at(g_timer->expires_at() + g_render_interval);
        g_timer->async_wait(&render);

        bgfx::reset(g_window_width, g_window_height, BGFX_RESET_MSAA_X4);
        drawUserInterface(g_window_width, g_window_height, g_main_menu_open);
        g_renderer->render(g_window_width, g_window_height);
        ++g_fps;
    }
}

static void fpsCount(const boost::system::error_code& error) {
    if (g_is_running) {
        g_fps_timer->expires_at(g_fps_timer->expires_at() + FPS_COUNTING_INTERVAL);
        g_fps_timer->async_wait(&fpsCount);

        float fps = FPS_MULTIPLIER * g_fps;
        float wait_time = static_cast<float>(g_render_interval.total_microseconds()) * fps;
        float render_time = std::max<float>(ONE_SECOND_IN_MC - wait_time, 0.0f);
        fps = std::max<float>(fps, 0.1f);
        float increase_rate = TARGET_FPS / fps;
        float new_render_time = increase_rate * render_time;
        float new_wait_time = std::max<float>(ONE_SECOND_IN_MC - new_render_time, 0.0f);
        float frame_wait_time = std::max<float>(new_wait_time / TARGET_FPS, 0.0f);
        g_render_interval = boost::posix_time::microseconds(static_cast<unsigned>(frame_wait_time));
        g_fps = 0;
    }
}

static void renderThread() {
    g_renderer = std::make_unique<Renderer>(g_window_width, g_window_height, g_hwnd, bgfx::RendererType::Direct3D11);
    g_renderer->init();
    imguiCreate();

    g_io_service = std::make_unique<boost::asio::io_service>();

    g_timer = std::make_unique<boost::asio::deadline_timer>(*g_io_service, g_render_interval);
    g_timer->async_wait(&render);

    g_fps_timer = std::make_unique<boost::asio::deadline_timer>(*g_io_service, FPS_COUNTING_INTERVAL);
    g_fps_timer->async_wait(&fpsCount);

    g_io_service->run();

    finishTessellationThreads();
    finishBlockOperationThread();
    finishGameLogicThread();

    imguiDestroy();
    g_world = nullptr;
    g_renderer = nullptr;
}

void initializeRenderer(HWND hwnd) {
    g_hwnd = hwnd;
    g_render_thread = std::make_unique<std::thread>(&renderThread);
}

void shutdownRenderer() {
    g_render_thread->join();
}

Renderer::Renderer(std::uint32_t width_, std::uint32_t height_, void* native_windows_handle, bgfx::RendererType::Enum render_type) {
    width = width_;
    height = height_;
    debug = BGFX_DEBUG_TEXT;
    reset = BGFX_RESET_NONE;

    bgfx::Init init;
    init.type = render_type;
    init.vendorId = BGFX_PCI_ID_NONE;
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = reset;
    init.platformData.nwh = native_windows_handle;
    bgfx::init(init);

    bgfx::setDebug(debug);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xbababaff, 1.0f, 0);

    BgfxVertex::init();

    draw_ref_info = std::make_shared<DrawRefInfo>();
    draw_ref_info->program = loadProgram();

    draw_ref_info->texture = makeBgfxSharedPtr(bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler));
    texture_cache = std::make_unique<TextureCache>("textures");

    for (unsigned i = 1; i < MATERIAL_MAX; ++i) {
        auto cur_texture = texture_cache->getTexture(i).bgfx_texture;
        draw_ref_info->material_textures[i] = cur_texture;
    }
}

void Renderer::init() {
    terrain = std::make_unique<FlatTerrain>(texture_cache->getTexture(1).bgfx_texture);
    intializeWorld();
}

TextureCache* Renderer::getTextureCache() const {
    return texture_cache.get();
}

template <std::uint8_t Level>
void drawInstances(DrawInfo draw_info, const std::list<typename Block<Level>::Ptr>& Blocks) {
    for (auto& block : Blocks) {
        auto block_draw_info = block->draw_info.read();
        if (block_draw_info) {
            constexpr std::uint16_t INSTANCE_STRIDE = 16;
            std::uint32_t total_blocks = static_cast<std::uint32_t>(block->draw_instance_info.size());
            std::uint32_t drawn_blocks = bgfx::getAvailInstanceDataBuffer(total_blocks, INSTANCE_STRIDE);
            bgfx::InstanceDataBuffer instance_buffer_data;
            bgfx::allocInstanceDataBuffer(&instance_buffer_data, drawn_blocks, INSTANCE_STRIDE);
            auto idb = reinterpret_cast<std::uint8_t*>(instance_buffer_data.data);
            auto instance_data_it = block->draw_instance_info.begin();
            for (std::uint32_t i = 0; i < drawn_blocks; ++i) {
                float* offset = reinterpret_cast<float*>(idb);
                offset[0] = static_cast<float>(instance_data_it->x);
                offset[1] = static_cast<float>(instance_data_it->y);
                offset[2] = static_cast<float>(instance_data_it->z);
                offset[3] = 1.0f;
                idb += INSTANCE_STRIDE;
                ++instance_data_it;
            }
            bgfx::setInstanceDataBuffer(&instance_buffer_data);
            bgfx::setState(draw_info.state);
            for (auto& command : block_draw_info->commands) {
                bgfx::setVertexBuffer(0, *command.vertex_buffer);
                bgfx::setTexture(0, *draw_info.ref_info->texture, *draw_info.ref_info->material_textures[command.material]);
                bgfx::submit(0, *draw_info.ref_info->program);
            }
            block->draw_instance_info.clear();
        }
    }
}

template <std::uint8_t Level>
struct BlockInstanceRenderInfo;

template <>
struct BlockInstanceRenderInfo<0> {
    std::list<Block<0>::Ptr> Blocks;
};

template <std::uint8_t Level>
struct BlockInstanceRenderInfo : public BlockInstanceRenderInfo<Level - 1> {
    std::list<typename Block<Level>::Ptr> Blocks;
};

template <std::uint8_t Level>
void collectInstances(
    BlockInstanceRenderInfo<Level>& info,
    const typename Block<Level>::Ptr& block,
    BlockIndexType base_x, BlockIndexType base_y, BlockIndexType base_z) {
    if (!block) {
        return;
    }
    if (block->entire && block->material == 0) {
        // The entire block is empty, skip it
        return;
    }
    if (block->draw_info.read()) {
        BlockMaterialType material = block->material;
        if (block->draw_instance_info.size() == 0) {
            // First time
            info.Blocks.push_back(block);
        }
        block->draw_instance_info.push_back({ base_x, base_y, base_z });
    } else {
        if constexpr (Level > 0) {
            constexpr BlockIndexType SUB_BLOCK_SIZE = Block<Level - 1>::SIZE;
            for (BlockIndexType z = 0; z < NESTED_BLOCKS; ++z) {
                for (BlockIndexType y = 0; y < NESTED_BLOCKS; ++y) {
                    for (BlockIndexType x = 0; x < NESTED_BLOCKS; ++x) {
                        Block<Level - 1>::Ptr child = block->children[z * NESTED_BLOCKS * NESTED_BLOCKS + y * NESTED_BLOCKS + x].read();
                        collectInstances<Level - 1>(info, child, base_x + x * SUB_BLOCK_SIZE, base_y + y * SUB_BLOCK_SIZE, base_z + z * SUB_BLOCK_SIZE);
                    }
                }
            }
        }
        TessellationRequest<Level> tessellation_request;
        tessellation_request.block = block;
        postBlockTessellationRequest(tessellation_request);
    }
}

template <std::uint8_t Level>
void drawInstances(DrawInfo draw_info, const BlockInstanceRenderInfo<Level>& info) {
    drawInstances<Level>(draw_info, info.Blocks);
    if constexpr (Level > 0) {
        drawInstances<Level - 1>(draw_info, info);
    }
}

template struct BlockInstanceRenderInfo<TOP_LEVEL>;
typedef BlockInstanceRenderInfo<TOP_LEVEL> TopLevelBlockInstanceRenderInfo;

void Renderer::render(int window_width, int window_height) {
    auto player_coordinates = g_player_coordinates.read();
    float player_direction = player_coordinates.direction * GKM_GRAD_TO_RAD;
    float player_pitch = player_coordinates.pitch * GKM_GRAD_TO_RAD;
    float xy_offset = cos(player_pitch) * 100.0f;
    float at_x_pos = player_coordinates.x + sin(player_direction) * xy_offset;
    float at_y_pos = player_coordinates.y + cos(player_direction) * xy_offset;
    float at_z_pos = sin(player_pitch) * 100.0f;

    float view_matrix[16];
    bx::mtxLookAt(
        view_matrix,
        bx::Vec3(static_cast<float>(player_coordinates.x), static_cast<float>(player_coordinates.y), TopLevelBlock::SIZE + 160.0f),
        bx::Vec3(at_x_pos, at_y_pos, at_z_pos + TopLevelBlock::SIZE + 160.0f), // Player's eyes height is 160 centimeters
        bx::Vec3(0.0f, 0.0f, 1.0f),
        bx::Handedness::Right
    );

    float projection_matrix[16];
    bx::mtxProj(
        projection_matrix, 30.0f,
        static_cast<float>(window_width) / static_cast<float>(window_height),
        1.0f,
        VIEW_DISTANCE * TopLevelBlock::SIZE,
        bgfx::getCaps()->homogeneousDepth,
        bx::Handedness::Right
    );
    bgfx::setViewTransform(0, view_matrix, projection_matrix);
    bgfx::setViewRect(0, 0, 0, window_width, window_height);

    bgfx::touch(0);
    DrawInfo draw_info;
    draw_info.ref_info = draw_ref_info.get();
    draw_info.state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW;
    //terrain->draw(draw_info);

    const bgfx::Caps* caps = bgfx::getCaps();
    const bool instancing_supported = 0 != (BGFX_CAPS_INSTANCING & caps->supported);
    if (instancing_supported) {
        TopLevelBlockInstanceRenderInfo info;
        BlockIndexType local_x;
        BlockIndexType local_y;
        BlockIndexType local_z;
        const BlockIndexType block_x_index = globalCoordinateToTopLevelBlockIndex(player_coordinates.x, local_x);
        const BlockIndexType block_y_index = globalCoordinateToTopLevelBlockIndex(player_coordinates.y, local_y);
        const BlockIndexType block_z_index = globalCoordinateToTopLevelBlockIndex(TopLevelBlock::SIZE, local_z);
        BlockIndexType start_block_x_index = block_x_index - VIEW_DISTANCE;
        BlockIndexType start_block_y_index = block_y_index - VIEW_DISTANCE;
        BlockIndexType finish_block_x_index = block_x_index + VIEW_DISTANCE;
        BlockIndexType finish_block_y_index = block_y_index + VIEW_DISTANCE;

        constexpr BlockIndexType NORTH_EAST = 45;
        constexpr BlockIndexType SOUTH_EAST = 135;
        constexpr BlockIndexType SOUTH_WEST = 225;
        constexpr BlockIndexType NORTH_WEST = 315;

        // By default draw all blocks which are nearer than VIEW_DISTANCE distance.
        // & - means the player position
        //
        // +------+------+
        // |      |      |
        // |      |      |
        // +------&------+
        // |      |      |
        // |      |      |
        // +------+------+

        if (player_coordinates.direction > NORTH_EAST && player_coordinates.direction <= SOUTH_EAST) {
            //start_block_x_index = block_x_index;
            // If player looks to the East then draw only half of the all blocks.
            // * - means the drawn block
            // +------********
            // |      ********
            // |      ********
            // +------********
            // |      ********
            // |      ********
            // +------********
        } else if (player_coordinates.direction > SOUTH_EAST && player_coordinates.direction <= SOUTH_WEST) {
            //finish_block_y_index = block_y_index;
            // If player looks to the South then draw only half of the all blocks.
            // * - means the drawn block
            // +------+------+
            // |      |      |
            // |      |      |
            // ***************
            // ***************
            // ***************
            // ***************
        } else if (player_coordinates.direction > SOUTH_WEST && player_coordinates.direction <= NORTH_WEST) {
            //finish_block_x_index = block_x_index;
            // If player looks to the West then draw only half of the all blocks.
            // * - means the drawn block
            // ********------+
            // ********      |
            // ********      |
            // ********------+
            // ********      |
            // ********      |
            // ********------+
        } else {
            //start_block_y_index = block_y_index;
            // If player looks to the North then draw only half of the all blocks.
            // * - means the drawn block
            // ***************
            // ***************
            // ***************
            // ***************
            // |      |      |
            // |      |      |
            // +------+------+
        }

        for (BlockIndexType x = start_block_x_index; x <= finish_block_x_index; ++x) {
            WorldLineY::Ptr cur_world_line = g_world->getLineByAbsoluteIndex(x);
            if (cur_world_line) {
                for (BlockIndexType y = start_block_y_index; y <= finish_block_y_index; ++y) {
                    WorldColumn::Ptr cur_world_column = cur_world_line->getColumnByAbsoluteIndex(y);
                    if (cur_world_column) {
                        for (BlockIndexType z = 0; z < WORLD_BLOCK_HEIGHT; ++z) {
                            TopLevelBlock::Ptr block = cur_world_column->getBlock(z);
                            collectInstances<TOP_LEVEL>(info, block, x * TopLevelBlock::SIZE, y * TopLevelBlock::SIZE, z * TopLevelBlock::SIZE);
                        }
                    }
                }
            }
        }
        drawInstances<TOP_LEVEL>(draw_info, info);
    }

    bgfx::frame();
}
