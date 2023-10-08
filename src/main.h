// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <atomic>
#include "win_api.h"
#include "texture_cache.h"

static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);

extern std::atomic<bool> g_is_running;

extern std::atomic<bool> g_left_mouse_pressed;
extern std::atomic<bool> g_right_mouse_pressed;
extern std::atomic<int> g_current_mouse_x;
extern std::atomic<int> g_current_mouse_y;
extern std::atomic<int> g_current_mouse_z;
extern std::atomic<int> g_previous_mouse_x;
extern std::atomic<int> g_previous_mouse_y;

extern std::atomic<bool> g_key_up_pressed;
extern std::atomic<bool> g_key_down_pressed;
extern std::atomic<bool> g_key_left_pressed;
extern std::atomic<bool> g_key_right_pressed;

extern std::atomic<bool> g_main_menu_open;
extern std::atomic<int> g_window_width;
extern std::atomic<int> g_window_height;
