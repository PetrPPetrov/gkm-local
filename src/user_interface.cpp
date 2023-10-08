// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include "main.h"
#include "user_interface.h"

void drawUserInterface(int window_width, int window_height, bool main_menu_open) {
    imguiBeginFrame(
        g_current_mouse_x,
        g_current_mouse_y,
        (g_left_mouse_pressed ? IMGUI_MBUT_LEFT : 0) | (g_right_mouse_pressed ? IMGUI_MBUT_RIGHT : 0),
        g_current_mouse_z,
        window_width,
        window_height,
        -1
    );

    const bgfx::Stats* statistic = bgfx::getStats();
    const double ms_cpu = 1000.0 / statistic->cpuTimerFreq;
    const double ms_gpu = 1000.0 / statistic->gpuTimerFreq;
    const double ms_frame = double(statistic->cpuTimeFrame) * ms_cpu;

    if (main_menu_open) {
        const float menu_width = 300.0f;
        const float menu_height = 240.0f;
        ImGui::SetNextWindowSize(ImVec2(menu_width, menu_height), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2((window_width - menu_width) / 2.0f, (window_height - menu_height) / 2.0f), ImGuiCond_Once);

        ImGui::Begin("Gkm-Local Menu", &main_menu_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

        ImGui::Text("Submit CPU %0.3f, GPU %0.3f (L: %d)"
                    , double(statistic->cpuTimeEnd - statistic->cpuTimeBegin) * ms_cpu
                    , double(statistic->gpuTimeEnd - statistic->gpuTimeBegin) * ms_gpu
                    , statistic->maxGpuLatency
        );

        if (-INT64_MAX != statistic->gpuMemoryUsed) {
            char tmp0[64];
            bx::prettify(tmp0, BX_COUNTOF(tmp0), statistic->gpuMemoryUsed);

            char tmp1[64];
            bx::prettify(tmp1, BX_COUNTOF(tmp1), statistic->gpuMemoryMax);

            ImGui::Text("GPU mem: %s / %s", tmp0, tmp1);
        }

        if (ImGui::Button("Exit")) {
            g_is_running = false;
        }
        ImGui::End();
    }

    imguiEndFrame();
}
