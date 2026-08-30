module;

#include <stb_image.h>

module Texture;

import Log;
import FileUtils;
import HeapManager;
import GTexture;

namespace rke
{
    Texture::Texture(const Path& filepath)
    {
        if(filepath.empty() || !filepath.exists()) {
            CORE_ERROR(u8"Texture: File '{}' not found!", filepath);
            return;
        }

        Buffer file_data{ file::read_file_binary(filepath) };
        if(file_data.empty()) {
            CORE_ERROR(u8"Texture: File '{}' empty or not readable!", filepath);
            return;
        }

        int width{}, height{}, channels{};
        stbi_set_flip_vertically_on_load(1);
        stbi_uc* data{ stbi_load_from_memory
        (
            file_data.data(),
            static_cast<int>(file_data.size()),
            &width, &height, &channels, 0
        )};
        stbi_set_flip_vertically_on_load(0);
        if(!data) {
            CORE_ERROR(u8"Texture: Failed to decode '{}'!", filepath);
            return;
        }

        width_ = static_cast<uint32>(width);
        height_ = static_cast<uint32>(height);
        channels_ = static_cast<uint32>(channels);
        pixels_.assign(data, data + width * height * channels);

        stbi_image_free(data);
    }

    GTexture* Texture::get_gtexture(const GTextureSettings& settings)
    {
        auto it{ gpu_variants_.find(settings) };
        if(it != gpu_variants_.end()) return it->second.get();

        GTexture::Format format{ GTexture::Format::None };
        switch(channels_)
        {
        case 4: format = settings.srgb ?
            GTexture::Format::SRGB8_ALPHA8 : GTexture::Format::RGBA8; break;
        case 3: format = settings.srgb ?
            GTexture::Format::SRGB8 : GTexture::Format::RGB8; break;
        case 1: format = GTexture::Format::R8; break;
        default:
            CORE_ERROR(u8"Texture: Unsupported channel count '{}'!", channels_);
            return nullptr;
        }

        if(!width_ || !height_ || pixels_.empty()) {
            CORE_ERROR(u8"Texture: Texture data invalid!");
            return nullptr;
        }

        Scope<GTexture> gpu{ GTexture2D::create (
            width_, height_, format,
            pixels_.data(), settings.filt, settings.wrap
        )};
        CORE_ASSERT(gpu, u8"Texture: Failed to upload GPU texture!");

        GTexture* raw{ gpu.get() };
        gpu_variants_.emplace(settings, std::move(gpu));
        return raw;
    }
}
