# Copyright 2023 Petr Petrov. All rights reserved.
# License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

project(gkm_local)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(RUN_AREA_DIR ${CMAKE_CURRENT_LIST_DIR}/run_area)

add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)

if(WIN32)
  # Note: using _WIN32_WINNT symbol here causes unpredictable ImGui font corruption (win_client)
  add_definitions(-D_WIN32_WINDOWS=_WIN32_WINNT_WIN7)
  add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

if(MSVC)
  add_definitions(/MP)
endif()

set(BGFX_ROOT CACHE PATH BGFX_ROOT)
if(NOT BGFX_ROOT)
  message(FATAL_ERROR "Need to specify BGFX_ROOT")
endif()

set(VCPKG_ROOT CACHE PATH VCPKG_ROOT)
if(NOT VCPKG_ROOT)
  message(FATAL_ERROR "Need to specify VCPKG_ROOT")
endif()

add_definitions(-D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
add_definitions(-DBOOST_ASIO_SEPARATE_COMPILATION)

if (MSVC)
  add_compile_options(/Zc:__cplusplus)
endif()

include_directories(
${PROJECT_SOURCE_DIR}/src
${PROJECT_SOURCE_DIR}/shaders
${BGFX_ROOT}/include
${BGFX_ROOT}/3rdparty
${BGFX_ROOT}/../bimg/include
${BGFX_ROOT}/../bx/include
${BGFX_ROOT}/../bx/include/compat/msvc
${BGFX_ROOT}/examples/common
${VCPKG_ROOT}/installed/x64-windows-static/include
${CMAKE_CURRENT_BINARY_DIR}
)

link_directories(${BGFX_ROOT}/.build/win64_vs2022/bin)
link_directories(${VCPKG_ROOT}/installed/x64-windows-static)

set(SHADER_H_FILES)
set(SHADER_SOURCES
${PROJECT_SOURCE_DIR}/shaders/varying.def.sc
${PROJECT_SOURCE_DIR}/shaders/constants.sh
)

macro(compile_shader_for_single_platform shader_file_name)
  list(APPEND SHADER_H_FILES ${CMAKE_CURRENT_BINARY_DIR}/${shader_file_name}.h)
  list(APPEND SHADER_SOURCES ${PROJECT_SOURCE_DIR}/shaders/${shader_file_name}.sc)
  set(shader_flags ${ARGN})
  add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${shader_file_name}.h
  COMMAND
    ${BGFX_ROOT}/.build/win64_vs2022/bin/shadercRelease
    -f ${PROJECT_SOURCE_DIR}/shaders/${shader_file_name}.sc
    -o ${CMAKE_CURRENT_BINARY_DIR}/${shader_file_name}.h
    --bin2c
    -i ${PROJECT_SOURCE_DIR}/shaders
    -i ${BGFX_ROOT}/src
    --varyingdef ${PROJECT_SOURCE_DIR}/shaders/varying.def.sc
    ${shader_flags}
  MAIN_DEPENDENCY
    ${PROJECT_SOURCE_DIR}/shaders/${shader_file_name}.sc
  DEPENDS
    ${PROJECT_SOURCE_DIR}/shaders/varying.def.sc
    ${PROJECT_SOURCE_DIR}/shaders/constants.sh
  WORKING_DIRECTORY
    ${PROJECT_SOURCE_DIR}/shaders
  )
endmacro()

macro(compile_vs_shader shader_file_name)
  compile_shader_for_single_platform(${shader_file_name} --type vertex --platform windows -p s_5_0 -O 3)
endmacro()

macro(compile_fs_shader shader_file_name)
  compile_shader_for_single_platform(${shader_file_name} --type fragment --platform windows -p s_5_0 -O 3)
endmacro()

compile_vs_shader(vs_pos_tex)
compile_fs_shader(fs_unlit)

add_executable(${PROJECT_NAME} WIN32
${PROJECT_SOURCE_DIR}/src/atomic_lock.h
${PROJECT_SOURCE_DIR}/src/mutex_free_compound.h
${PROJECT_SOURCE_DIR}/src/mutex_free_fifo.h
${PROJECT_SOURCE_DIR}/src/block.h
${PROJECT_SOURCE_DIR}/src/block.cpp
${PROJECT_SOURCE_DIR}/src/block_operation.h
${PROJECT_SOURCE_DIR}/src/block_operation.cpp
${PROJECT_SOURCE_DIR}/src/tessellation.h
${PROJECT_SOURCE_DIR}/src/tessellation.cpp
${PROJECT_SOURCE_DIR}/src/world.h
${PROJECT_SOURCE_DIR}/src/world.cpp
${PROJECT_SOURCE_DIR}/src/flat_terrain.h
${PROJECT_SOURCE_DIR}/src/flat_terrain.cpp
${PROJECT_SOURCE_DIR}/src/game_logic.h
${PROJECT_SOURCE_DIR}/src/game_logic.cpp
${PROJECT_SOURCE_DIR}/src/user_interface.h
${PROJECT_SOURCE_DIR}/src/user_interface.cpp
${PROJECT_SOURCE_DIR}/src/main.h
${PROJECT_SOURCE_DIR}/src/main.cpp
${PROJECT_SOURCE_DIR}/src/renderer.h
${PROJECT_SOURCE_DIR}/src/renderer.cpp
${PROJECT_SOURCE_DIR}/src/texture_cache.h
${PROJECT_SOURCE_DIR}/src/texture_cache.cpp
${PROJECT_SOURCE_DIR}/src/draw_info.h
${PROJECT_SOURCE_DIR}/src/shader.h
${PROJECT_SOURCE_DIR}/src/bgfx_api.h
${PROJECT_SOURCE_DIR}/src/win_api.h
${PROJECT_SOURCE_DIR}/src/fnv_hash.h
${SHADER_SOURCES}
${SHADER_H_FILES}
${BGFX_ROOT}/3rdparty/dear-imgui/imgui.cpp
${BGFX_ROOT}/3rdparty/dear-imgui/imgui_demo.cpp
${BGFX_ROOT}/3rdparty/dear-imgui/imgui_draw.cpp
${BGFX_ROOT}/3rdparty/dear-imgui/imgui_tables.cpp
${BGFX_ROOT}/3rdparty/dear-imgui/imgui_widgets.cpp
${BGFX_ROOT}/examples/common/imgui/imgui.cpp
)

set_property(TARGET gkm_local PROPERTY COMPILE_DEFINITIONS $<IF:$<CONFIG:DEBUG>,BX_CONFIG_DEBUG=1,BX_CONFIG_DEBUG=0>)

source_group(ShaderBinaries FILES ${SHADER_H_FILES})
source_group(ShadersSources FILES ${SHADER_SOURCES})
set_source_files_properties(${SHADER_H_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

target_link_libraries(${PROJECT_NAME} optimized bgfxRelease)
target_link_libraries(${PROJECT_NAME} debug bgfxDebug)

target_link_libraries(${PROJECT_NAME} optimized bimgRelease)
target_link_libraries(${PROJECT_NAME} debug bimgDebug)

target_link_libraries(${PROJECT_NAME} optimized bimg_decodeRelease)
target_link_libraries(${PROJECT_NAME} debug bimg_decodeDebug)

target_link_libraries(${PROJECT_NAME} optimized bimg_encodeRelease)
target_link_libraries(${PROJECT_NAME} debug bimg_encodeDebug)

target_link_libraries(${PROJECT_NAME} optimized bxRelease)
target_link_libraries(${PROJECT_NAME} debug bxDebug)

target_link_libraries(${PROJECT_NAME} optimized ${VCPKG_ROOT}/installed/x64-windows-static/lib/turbojpeg.lib)
target_link_libraries(${PROJECT_NAME} debug ${VCPKG_ROOT}/installed/x64-windows-static/debug/lib/turbojpeg.lib)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${RUN_AREA_DIR})
