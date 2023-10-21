// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include "gkm_local.h"
#include "spin_lock.h"

constexpr double GKM_PI = 3.14159265358979323846264338327950288;
constexpr double GKM_2PI = 2 * GKM_PI;
constexpr double GKM_GRAD_TO_RAD = GKM_PI / 180.0;
constexpr double GKM_SPEED = 5.0;

struct PlayerCoordinates {
    double x = 0;
    double y = 0;
    double direction = 0; // In degrees between 0 and 359
    double pitch = 0; // In degrees between -90 and 90

    typedef SpinLocked<PlayerCoordinates> Atomic;
};

extern PlayerCoordinates::Atomic g_player_coordinates;

struct DirectionPitchDelta {
    double direction = 0;
    double pitch = 0;
};

void startGameLogicThread();
void finishGameLogicThread();
void postDirectionPitchDelta(const DirectionPitchDelta& delta);

inline void normalizeDirection(double& direction) {
    while (direction >= 360.0) {
        direction -= 360.0;
    }
    while (direction < 0.0) {
        direction += 360.0;
    }
}

inline void normalizePitch(double& pitch) {
    if (pitch >= 80.0) {
        pitch = 80.0;
    }
    else if (pitch <= -80.0) {
        pitch = -80.0;
    }
}
