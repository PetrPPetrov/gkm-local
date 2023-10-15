// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <iostream>
#include "user_interface.h"
#include "game_logic.h"
#include "block_operation.h"
#include "tessellation.h"
#include "renderer.h"
#include "world.h"
#include "main.h"

std::atomic<bool> g_is_running = true;

std::atomic<bool> g_left_mouse_pressed = false;
std::atomic<bool> g_right_mouse_pressed = false;
std::atomic<int> g_current_mouse_x = 0;
std::atomic<int> g_current_mouse_y = 0;
std::atomic<int> g_current_mouse_z = 0;
std::atomic<int> g_previous_mouse_x = 0;
std::atomic<int> g_previous_mouse_y = 0;

std::atomic<bool> g_key_up_pressed = false;
std::atomic<bool> g_key_down_pressed = false;
std::atomic<bool> g_key_left_pressed = false;
std::atomic<bool> g_key_right_pressed = false;

std::atomic<bool> g_main_menu_open = false;
std::atomic<int> g_window_width = 800;
std::atomic<int> g_window_height = 600;

static HINSTANCE g_hinstance = 0;
static HWND g_hwnd = 0;

static int g_screen_width = 0;
static int g_screen_height = 0;
static bool g_first_mouse_move = true;

static void onLeftMouseButtonDown(LPARAM l_param) {
    g_current_mouse_x = GET_X_LPARAM(l_param);
    g_current_mouse_y = GET_Y_LPARAM(l_param);
    g_left_mouse_pressed = true;
}

static void onLeftMouseButtonUp(LPARAM l_param) {
    g_current_mouse_x = GET_X_LPARAM(l_param);
    g_current_mouse_y = GET_Y_LPARAM(l_param);
    g_left_mouse_pressed = false;
}

static void onRightMouseButtonDown(LPARAM l_param) {
    g_current_mouse_x = GET_X_LPARAM(l_param);
    g_current_mouse_y = GET_Y_LPARAM(l_param);
    g_right_mouse_pressed = true;
}

static void onRightMouseButtonUp(LPARAM l_param) {
    g_current_mouse_x = GET_X_LPARAM(l_param);
    g_current_mouse_y = GET_Y_LPARAM(l_param);
    g_right_mouse_pressed = false;
}

static void onMouseMove(LPARAM l_param) {
    g_current_mouse_x = GET_X_LPARAM(l_param);
    g_current_mouse_y = GET_Y_LPARAM(l_param);
    if (g_first_mouse_move) {
        g_previous_mouse_x.store(g_current_mouse_x);
        g_previous_mouse_y.store(g_current_mouse_y);
        g_first_mouse_move = false;
    } else {
        DirectionPitchDelta delta;
        delta.direction = static_cast<float>((g_current_mouse_x - g_previous_mouse_x));
        delta.pitch = static_cast<float>(-(g_current_mouse_y - g_previous_mouse_y));
        if (delta.direction != 0 || delta.pitch != 0) {
            postDirectionPitchDelta(delta);
        }
        auto mouse_x = g_window_width / 2;
        auto mouse_y = g_window_height / 2;
        POINT pt = { mouse_x, mouse_y };
        ClientToScreen(g_hwnd, &pt);
        SetCursorPos(pt.x, pt.y);
        g_previous_mouse_x.store(mouse_x);
        g_previous_mouse_y.store(mouse_y);
    }
}

static void onSizing(RECT* rect) {
    g_window_width = rect->right - rect->left;
    g_window_height = rect->bottom - rect->top;
}

static void onSize(LPARAM l_param) {
    g_window_width = GET_X_LPARAM(l_param);
    g_window_height = GET_Y_LPARAM(l_param);
}

static LRESULT APIENTRY CALLBACK onWindowEvent(HWND window_handle, UINT window_msg, WPARAM w_param, LPARAM l_param) {
    LRESULT result = 1;
    switch (window_msg) {
    case WM_LBUTTONDOWN:
        onLeftMouseButtonDown(l_param);
        break;
    case WM_LBUTTONUP:
        onLeftMouseButtonUp(l_param);
        break;
    case WM_RBUTTONDOWN:
        onRightMouseButtonDown(l_param);
        break;
    case WM_RBUTTONUP:
        onRightMouseButtonUp(l_param);
        break;
    case WM_MOUSEMOVE:
        onMouseMove(l_param);
        break;
    case WM_SIZING:
        onSizing(reinterpret_cast<RECT*>(l_param));
        break;
    case WM_SIZE:
        onSize(l_param);
        break;
    case WM_KEYDOWN:
    {
        switch (w_param) {
        case VK_ESCAPE:
            g_main_menu_open = !g_main_menu_open;
            break;
        case VK_UP:
        case 0x57:
            g_key_up_pressed = true;
            break;
        case VK_DOWN:
        case 0x53:
            g_key_down_pressed = true;
            break;
        case VK_LEFT:
        case 0x41:
            g_key_left_pressed = true;
            break;
        case VK_RIGHT:
        case 0x44:
            g_key_right_pressed = true;
            break;
        }
        break;
    case WM_KEYUP:
        switch (w_param) {
        case VK_UP:
        case 0x57:
            g_key_up_pressed = false;
            break;
        case VK_DOWN:
        case 0x53:
            g_key_down_pressed = false;
            break;
        case VK_LEFT:
        case 0x41:
            g_key_left_pressed = false;
            break;
        case VK_RIGHT:
        case 0x44:
            g_key_right_pressed = false;
            break;
        }
    }
    break;
    case WM_CLOSE:
        g_is_running = false;
        break;
    default:
        result = DefWindowProc(window_handle, window_msg, w_param, l_param);
        break;
    }
    return result;
}

static void registerWindowClass() {
    g_screen_width = GetSystemMetrics(SM_CXSCREEN);
    g_screen_height = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASS window_class;
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = (WNDPROC)onWindowEvent;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = g_hinstance;
    window_class.hIcon = LoadIcon(g_hinstance, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_CROSS);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = "Gkm-Local";

    if (!RegisterClass(&window_class)) {
        MessageBox(NULL, "Couldn't Register Window Class", "Error", MB_ICONERROR);
        exit(1);
    }
}

static HWND createWindow() {
    g_window_width = g_screen_width * 3 / 4;
    g_window_height = g_screen_height * 3 / 4;

    int window_coord_x = (g_screen_width - g_window_width) / 2;
    int window_coord_y = (g_screen_height - g_window_height) / 2;

    HWND window = CreateWindowEx(WS_EX_APPWINDOW, "Gkm-Local", "Gkm-Local", WS_OVERLAPPEDWINDOW, window_coord_x, window_coord_y, g_window_width, g_window_height, NULL, NULL, g_hinstance, NULL);
    MoveWindow(window, window_coord_x, window_coord_y, g_window_width, g_window_height, FALSE);
    return window;
}

static int winMainImpl(HINSTANCE h_instance, HINSTANCE h_prev_instance, LPSTR lp_cmd_line, INT n_cmd_show) {
    g_hinstance = h_instance;
    registerWindowClass();
    g_hwnd = createWindow();
    if (!g_hwnd) {
        MessageBox(NULL, "Couldn't Create Window", "Error", MB_ICONERROR);
        exit(1);
    }
    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);
    SetFocus(g_hwnd);
    initializeRenderer(g_hwnd);
    startGameLogicThread();
    startBlockOperationThread();
    startTessellationThreads();

    MSG window_event;
    while (g_is_running && GetMessage(&window_event, g_hwnd, 0, 0) > 0) {
        TranslateMessage(&window_event);
        DispatchMessage(&window_event);
    }

    // Render thread will shutdown and wait for all other threads
    shutdownRenderer();

    DestroyWindow(g_hwnd);
    return EXIT_SUCCESS;
}

int WINAPI WinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance, LPSTR lp_cmd_line, INT n_cmd_show) {
#if defined(_WIN32) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try {
        return winMainImpl(h_instance, h_prev_instance, lp_cmd_line, n_cmd_show);
    } catch (const std::exception& exception) {
        MessageBox(0, exception.what(), "std::exception was thrown", MB_OK);
        std::cout << "std::exception was thrown: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        MessageBox(0, "description is not available", "unknown exception was thrown", MB_OK);
        std::cout << "unknown exception was thrown" << std::endl;
        return EXIT_FAILURE;
    }
}
