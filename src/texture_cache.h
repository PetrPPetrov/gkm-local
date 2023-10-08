// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "bgfx_api.h"

struct TextureInfo {
    BgfxTexturePtr bgfx_texture;
    std::uint16_t texture_id = 0;
    std::uint64_t texture_hash = 0;
};

class TextureCache {
    static const int MAX_TEXTURE_SIZE = 4096;
    BgfxTexturePtr loadJpegRgbTexture(const std::vector<std::uint8_t>& data, const std::string& file_name);

public:
    typedef std::unique_ptr<TextureCache> Ptr;

    TextureCache(const std::string& texture_cache_dir);
    TextureInfo getTexture(std::uint16_t texture_id);

private:
    const std::string texture_cache_dir;
    std::unordered_map<std::uint16_t, TextureInfo> id_to_texture_info;
};
