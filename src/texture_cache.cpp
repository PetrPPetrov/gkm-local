// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#include <sstream>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include "turbojpeg.h"
#include "fnv_hash.h"
#include "texture_cache.h"

struct TurboJpegHandleHolder {
    TurboJpegHandleHolder(tjhandle tj_handle_) : tj_handle(tj_handle_) {
        if (!tj_handle) {
            throw std::runtime_error("Can not create TurboJpeg decompressor");
        }
    }
    ~TurboJpegHandleHolder() {
        tjDestroy(tj_handle);
    }
    operator tjhandle() const {
        return tj_handle;
    }
private:
    tjhandle tj_handle;
};

BgfxTexturePtr TextureCache::loadJpegRgbTexture(const std::vector<std::uint8_t>& data, const std::string& file_name) {
    // TODO: throw an exception if texture could not be loaded or use some default texture image

    TurboJpegHandleHolder decompressor(tjInitDecompress());
    int image_width = 0;
    int image_height = 0;
    int image_jpeg_subsamp = 0;
    int image_color_space = 0;
    int error_code = tjDecompressHeader3(
        decompressor,
        &data[0], static_cast<unsigned long>(data.size()),
        &image_width, &image_height, &image_jpeg_subsamp, &image_color_space);
    if (error_code) {
        throw std::runtime_error("Can not decompressed Jpeg header");
    }
    if (image_width == 0 || image_height == 0) {
        throw std::runtime_error("Jpeg image is empty");
    }
    if (image_width > MAX_TEXTURE_SIZE) {
        throw std::runtime_error("Jpeg image is too wide");
    }
    if (image_height > MAX_TEXTURE_SIZE) {
        throw std::runtime_error("Jpeg image is too tall");
    }
    std::vector<std::uint8_t> rgb8_buffer(static_cast<size_t>(image_width) * static_cast<size_t>(image_height) * 3);

    error_code = tjDecompress2(
        decompressor,
        &data[0], static_cast<unsigned long>(data.size()),
        &rgb8_buffer[0],
        image_width, 0, image_height, TJPF_RGB, 0);
    if (error_code) {
        throw std::runtime_error("Can not decompressed Jpeg image data");
    }

    const bimg::ImageBlockInfo& output_block_info = bimg::getBlockInfo(bimg::TextureFormat::RGB8);
    const std::uint32_t block_width = output_block_info.blockWidth;

    bx::Error err;
    bx::DefaultAllocator allocator;
    bimg::ImageContainer* output = bimg::imageAlloc(
        &allocator,
        bimg::TextureFormat::RGB8,
        static_cast<std::uint16_t>(image_width),
        static_cast<std::uint16_t>(image_height),
        1,
        1,
        false,
        true
    );
    std::uint8_t num_mips = output->m_numMips;

    bimg::ImageMip source_mip;
    source_mip.m_format = bimg::TextureFormat::RGB8;
    source_mip.m_width = static_cast<std::uint32_t>(image_width);
    source_mip.m_height = static_cast<std::uint32_t>(image_height);
    source_mip.m_depth = 1;
    source_mip.m_blockSize = 3;
    source_mip.m_size = static_cast<std::uint32_t>(rgb8_buffer.size());
    source_mip.m_bpp = 24;
    source_mip.m_hasAlpha = false;
    source_mip.m_data = &rgb8_buffer[0];

    bimg::ImageMip destination_mip;
    bimg::imageGetRawData(*output, 0, 0, output->m_data, output->m_size, destination_mip);

    std::uint32_t size = bimg::imageGetSize(
        nullptr,
        static_cast<std::uint16_t>(destination_mip.m_width),
        static_cast<std::uint16_t>(destination_mip.m_height),
        static_cast<std::uint16_t>(destination_mip.m_depth),
        false,
        false,
        1,
        bimg::TextureFormat::RGBA8
    );

    BxAlloc temp(&allocator, size);
    std::uint8_t* rgba = static_cast<uint8_t*>(temp.get());

    bimg::imageDecodeToRgba8(
        &allocator,
        rgba,
        source_mip.m_data,
        source_mip.m_width,
        source_mip.m_height,
        source_mip.m_width * 4,
        source_mip.m_format
    );

    bimg::imageGetRawData(*output, 0, 0, output->m_data, output->m_size, destination_mip);
    std::uint8_t* destination_data = const_cast<std::uint8_t*>(destination_mip.m_data);

    bimg::imageEncodeFromRgba8(
        &allocator,
        destination_data,
        rgba,
        destination_mip.m_width,
        destination_mip.m_height,
        destination_mip.m_depth,
        bimg::TextureFormat::RGB8,
        bimg::Quality::Default,
        &err
    );

    for (std::uint8_t lod = 1; lod < num_mips && err.isOk(); ++lod) {
        bimg::imageRgba8Downsample2x2(
            rgba,
            destination_mip.m_width,
            destination_mip.m_height,
            destination_mip.m_depth,
            destination_mip.m_width * 4,
            bx::strideAlign(destination_mip.m_width / 2, block_width) * 4,
            rgba
        );
        bimg::imageGetRawData(*output, 0, lod, output->m_data, output->m_size, destination_mip);
        std::uint8_t* destination_data = const_cast<std::uint8_t*>(destination_mip.m_data);
        bimg::imageEncodeFromRgba8(
            &allocator,
            destination_data,
            rgba,
            destination_mip.m_width,
            destination_mip.m_height,
            destination_mip.m_depth,
            bimg::TextureFormat::RGB8,
            bimg::Quality::Default,
            &err
        );
    }
    // TODO: avoid additional copy
    const bgfx::Memory* mem = bgfx::copy(
        output->m_data,
        output->m_size
    );
    BgfxTexturePtr result_texture = makeBgfxSharedPtr(bgfx::createTexture2D(
        static_cast<std::uint16_t>(output->m_width),
        static_cast<std::uint16_t>(output->m_height),
        1 < output->m_numMips,
        output->m_numLayers,
        bgfx::TextureFormat::Enum(output->m_format),
        0,
        mem
    ));
    // TODO: make RAII memory deallocations
    bimg::imageFree(output);
    return result_texture;
}

TextureCache::TextureCache(const std::string& texture_cache_dir_) : texture_cache_dir(texture_cache_dir_) {}

TextureInfo TextureCache::getTexture(std::uint16_t texture_id) {
    auto find_it = id_to_texture_info.find(texture_id);
    if (find_it == id_to_texture_info.end()) {
        TextureInfo result;
        std::stringstream file_name_in_cache;
        file_name_in_cache << texture_cache_dir << "/" << std::setw(8) << std::setfill('0') << texture_id << ".jpg";

        std::ifstream input_file(file_name_in_cache.str(), std::ifstream::binary);
        if (input_file) {
            input_file.seekg(0, input_file.end);
            std::size_t length = input_file.tellg();
            input_file.seekg(0, input_file.beg);

            if (length > 0) {
                std::vector<std::uint8_t> buffer(length);
                input_file.read(reinterpret_cast<char*>(&buffer[0]), length);
                input_file.close();

                FnvHash fnv_hash;
                fnv_hash.update(&buffer[0], length);
                result.texture_hash = fnv_hash.getHash();
                result.texture_id = texture_id;
                result.bgfx_texture = loadJpegRgbTexture(buffer, file_name_in_cache.str());
            }
        }
        find_it = id_to_texture_info.emplace(texture_id, result).first;
    }
    return find_it->second;
}
