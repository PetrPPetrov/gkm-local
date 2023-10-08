// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include "bx/bx.h"
#include "bx/allocator.h"
#include "bx/error.h"
#include "bx/math.h"
#include "bimg/bimg.h"
#include "bimg/encode.h"
#include "bgfx/bgfx.h"
#include "imgui/imgui.h"

template<class BgfxType>
struct BgfxHandleHolder {
    BgfxHandleHolder(BgfxType bgfx_handle_) : bgfx_handle(bgfx_handle_) {}
    ~BgfxHandleHolder() {
        bgfx::destroy(bgfx_handle);
    }
    operator BgfxType() const {
        return bgfx_handle;
    }
    bool isValid() const {
        return bgfx::isValid(bgfx_handle);
    }
private:
    BgfxType bgfx_handle;
};

template<class BgfxType>
using BgfxUniquePtr = std::unique_ptr<BgfxHandleHolder<BgfxType>>;

template<class BgfxType>
using BgfxSharedPtr = std::shared_ptr<BgfxHandleHolder<BgfxType>>;

template<class BgfxType>
BgfxUniquePtr<BgfxType> makeBgfxUniquePtr(BgfxType bgfx_handle) {
    return std::make_unique<BgfxHandleHolder<BgfxType>>(bgfx_handle);
}

template<class BgfxType>
BgfxSharedPtr<BgfxType> makeBgfxSharedPtr(BgfxType bgfx_handle) {
    return std::make_shared<BgfxHandleHolder<BgfxType>>(bgfx_handle);
}

typedef BgfxSharedPtr<bgfx::VertexBufferHandle> BgfxVertexBufferPtr;
typedef BgfxSharedPtr<bgfx::IndexBufferHandle> BgfxIndexBufferPtr;
typedef BgfxSharedPtr<bgfx::ShaderHandle> BgfxShaderPtr;
typedef BgfxSharedPtr<bgfx::ProgramHandle> BgfxProgramPtr;
typedef BgfxSharedPtr<bgfx::UniformHandle> BgfxUniformPtr;
typedef BgfxSharedPtr<bgfx::TextureHandle> BgfxTexturePtr;

struct BgfxEngineShutdown {
    ~BgfxEngineShutdown() {
        bgfx::shutdown();
    }
};

struct BxAlloc {
    BxAlloc(bx::AllocatorI* allocator_, std::uint32_t size) : allocator(allocator_) {
        allocated_data = bx::alloc(allocator, size);
    }
    ~BxAlloc() {
        bx::free(allocator, allocated_data);
    }
    void* get() const {
        return allocated_data;
    }
private:
    bx::AllocatorI* allocator;
    void* allocated_data;
};

#pragma pack(push, 1)

struct BgfxVertex {
    float x, y, z;
    std::int16_t u, v;
    std::int16_t offset_u_mode, offset_v_mode;

    static void init() {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Int16)
            .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Int16)
            .end();
    }

    static bgfx::VertexLayout ms_layout;
};

#pragma pack(pop)
