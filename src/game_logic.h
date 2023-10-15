// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include "gkm_local.h"
#include "spin_lock.h"

constexpr float GKM_PI = 3.14159265358979323846264338327950288f;
constexpr float GKM_2PI = 2 * GKM_PI;
constexpr float GKM_GRAD_TO_RAD = GKM_PI / 180.0f;
constexpr float GKM_SPEED = 5.0f;

struct PlayerCoordinates {
    BlockIndexType x = 0;
    BlockIndexType y = 0;
    float direction = 0; // In degrees between 0 and 359
    float pitch = 0; // In degrees between -90 and 90

    typedef SpinLocked<PlayerCoordinates> Atomic;
};

extern PlayerCoordinates::Atomic g_player_coordinates;

struct DirectionPitchDelta {
    float direction = 0;
    float pitch = 0;
};

void startGameLogicThread();
void finishGameLogicThread();
void postDirectionPitchDelta(const DirectionPitchDelta& delta);

inline void normalizeDirection(float& direction) {
    while (direction >= 360.0f) {
        direction -= 360.0f;
    }
    while (direction < 0.0f) {
        direction += 360.0f;
    }
}

inline void normalizePitch(float& pitch) {
    if (pitch >= 80.0f) {
        pitch = 80.0f;
    }
    else if (pitch <= -80.0f) {
        pitch = -80.0f;
    }
}
