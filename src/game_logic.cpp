// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include <thread>
#include <memory>
#include <boost/asio/impl/src.hpp>
#include <boost/asio.hpp>
#include "main.h"
#include "request_queue.h"
#include "block_operation.h"
#include "game_logic.h"

PlayerCoordinates::Atomic g_player_coordinates;

static RequestQueue<DirectionPitchDelta> g_direction_pitch_delta_queue;

static std::unique_ptr<std::thread> g_game_logic_thread = nullptr;
static std::unique_ptr<boost::asio::io_service> g_io_service;
static std::unique_ptr<boost::asio::deadline_timer> g_timer;

static std::uint32_t g_tick = 0;

static void gameStep(const boost::system::error_code& error) {
    auto player_coordinates = g_player_coordinates.read();
    DirectionPitchDelta delta;
    while (g_direction_pitch_delta_queue.pop(delta)) {
        player_coordinates.direction += delta.direction;
        player_coordinates.direction %= 360;
        if (player_coordinates.direction < 0) {
            player_coordinates.direction += 360;
        }
        player_coordinates.pitch += delta.pitch;
        if (player_coordinates.pitch < -80) {
            player_coordinates.pitch = -80;
        } else if (player_coordinates.pitch > 80) {
            player_coordinates.pitch = 80;
        }
    }
    float direction = 0.0f;
    if (g_key_up_pressed && !g_key_down_pressed) {
        direction = 1.0f;
    } else if (!g_key_up_pressed && g_key_down_pressed) {
        direction = -1.0f;
    }
    float player_direction = player_coordinates.direction * GKM_GRAD_TO_RAD;
    player_coordinates.x += static_cast<GlobalCoordinateType>(sin(player_direction) * direction * GKM_SPEED);
    player_coordinates.y += static_cast<GlobalCoordinateType>(cos(player_direction) * direction * GKM_SPEED);
    direction = 0.0f;
    if (g_key_left_pressed && !g_key_right_pressed) {
        direction = -1.0f;
    } else if (!g_key_left_pressed && g_key_right_pressed) {
        direction = 1.0f;
    }
    auto right_direction = player_coordinates.direction + 90;
    right_direction %= 360;
    float right_dir = right_direction * GKM_GRAD_TO_RAD;
    player_coordinates.x += static_cast<GlobalCoordinateType>(sin(right_dir) * direction * GKM_SPEED);
    player_coordinates.y += static_cast<GlobalCoordinateType>(cos(right_dir) * direction * GKM_SPEED);
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

    if (g_is_running) {
        g_timer->expires_at(g_timer->expires_at() + GAME_TIME_INTERVAL);
        g_timer->async_wait(&gameStep);
    }
}

static void gameLogicThread() {
    g_io_service = std::make_unique<boost::asio::io_service>();
    g_timer = std::make_unique<boost::asio::deadline_timer>(*g_io_service, GAME_TIME_INTERVAL);
    g_timer->async_wait(&gameStep);
    g_io_service->run();
}

void startGameLogicThread() {
    g_game_logic_thread = std::make_unique<std::thread>(&gameLogicThread);
}

void finishGameLogicThread() {
    g_game_logic_thread->join();
}

void postDirectionPitchDelta(const DirectionPitchDelta& delta) {
    g_direction_pitch_delta_queue.push(delta);
}
