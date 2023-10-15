// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include <thread>
#include <memory>
#include "main.h"
#include "request_queue.h"
#include "block_operation.h"
#include "game_logic.h"

PlayerCoordinates::Atomic g_player_coordinates;

static RequestQueue<DirectionPitchDelta> g_direction_pitch_delta_queue;

static std::unique_ptr<std::thread> g_game_logic_thread = nullptr;

static std::uint32_t g_tick = 0;

static void gameLogicThread() {
    while (g_is_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        auto player_coordinates = g_player_coordinates.read();
        DirectionPitchDelta delta;
        while (g_direction_pitch_delta_queue.pop(delta)) {
            player_coordinates.direction += delta.direction;
            normalizeDirection(player_coordinates.direction);
            player_coordinates.pitch += delta.pitch;
            normalizePitch(player_coordinates.pitch);
        }
        float direction = 0.0f;
        if (g_key_up_pressed && !g_key_down_pressed) {
            direction = 1.0f;
        }
        else if (!g_key_up_pressed && g_key_down_pressed) {
            direction = -1.0f;
        }
        const float player_direction = player_coordinates.direction * GKM_GRAD_TO_RAD;
        player_coordinates.x += static_cast<BlockIndexType>(sin(player_direction) * direction * GKM_SPEED);
        player_coordinates.y += static_cast<BlockIndexType>(cos(player_direction) * direction * GKM_SPEED);
        direction = 0.0f;
        if (g_key_left_pressed && !g_key_right_pressed) {
            direction = -1.0f;
        }
        else if (!g_key_left_pressed && g_key_right_pressed) {
            direction = 1.0f;
        }
        auto right_direction = player_coordinates.direction + 90;
        normalizeDirection(right_direction);
        const float right_dir = right_direction * GKM_GRAD_TO_RAD;
        player_coordinates.x += static_cast<BlockIndexType>(sin(right_dir) * direction * GKM_SPEED);
        player_coordinates.y += static_cast<BlockIndexType>(cos(right_dir) * direction * GKM_SPEED);
        g_player_coordinates.write(player_coordinates);

        //constexpr std::uint32_t BASE_TICK = 3 * 20; // Wait 10 seconds
        //static bool enable_building = true;
        //if (g_tick >= BASE_TICK && enable_building) {
        //    static std::uint32_t cur_x = 0;
        //    static std::uint32_t cur_y = 0;
        //    static std::uint32_t cur_z = 0;

        //    BlockOperation put_block_request;
        //    put_block_request.put_block = true;
        //    put_block_request.material = 2;
        //    put_block_request.x = cur_x;
        //    put_block_request.y = TopLevelBlock::SIZE + cur_y;
        //    put_block_request.z = TopLevelBlock::SIZE + cur_z;
        //    postBlockOperation(put_block_request);

        //    static std::uint32_t LIMIT_X = 20;
        //    static std::uint32_t LIMIT_Y = 20;
        //    static std::uint32_t LIMIT_Z = 10;

        //    ++cur_x;
        //    if (cur_x >= LIMIT_X) {
        //        cur_x = 0;
        //        ++cur_y;
        //        if (cur_y >= LIMIT_Y) {
        //            cur_y = 0;
        //            ++cur_z;
        //            --LIMIT_X;
        //            --LIMIT_Y;
        //            if (cur_z >= LIMIT_Z) {
        //                enable_building = false;
        //            }
        //        }
        //    }
        //}
        //++g_tick;
    }
}

void startGameLogicThread() {
    g_game_logic_thread = std::make_unique<std::thread>(&gameLogicThread);
}

void finishGameLogicThread() {
    g_game_logic_thread->join();
    g_game_logic_thread.reset();
}

void postDirectionPitchDelta(const DirectionPitchDelta& delta) {
    g_direction_pitch_delta_queue.push(delta);
}
