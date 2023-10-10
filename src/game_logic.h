// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "gkm_local.h"
#include "spin_lock.h"

constexpr float GKM_PI = 3.14159265358979323846264338327950288f;
constexpr float GKM_2PI = 2 * GKM_PI;
constexpr float GKM_GRAD_TO_RAD = GKM_PI / 180.0f;
constexpr float GKM_SPEED = 5.0f;

const boost::posix_time::milliseconds GAME_TIME_INTERVAL(25);

struct PlayerCoordinates {
    BlockIndexType x = 0;
    BlockIndexType y = 0;
    BlockIndexType direction = 0; // In degrees between 0 and 359
    BlockIndexType pitch = 0; // In degrees between -90 and 90

    typedef SpinLocked<PlayerCoordinates> Atomic;
};

extern PlayerCoordinates::Atomic g_player_coordinates;

struct DirectionPitchDelta {
    int direction = 0;
    int pitch = 0;
};

void startGameLogicThread();
void finishGameLogicThread();
void postDirectionPitchDelta(const DirectionPitchDelta& delta);
