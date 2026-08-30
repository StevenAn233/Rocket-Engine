module;

#include <vector>
#include <unordered_map>
#include "rke_macros.h"

export module Texture;

import Types;
import HeapManager;
import String;
import Path;
import GTexture;

export namespace rke
{
    class RKE_API Texture
    {
    public:
        Texture(const Path& filepath);
        ~Texture() = default;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = default;
        Texture& operator=(Texture&&) = default;

        uint32 get_width() const { return width_; }
        uint32 get_height() const { return height_; }
        uint32 get_channels() const { return channels_; }

        // lazily uploads & caches a GPU variant for the given settings
        GTexture* get_gtexture(const GTextureSettings& settings);
    private:
        std::vector<byte> pixels_{};
        uint32 width_{}, height_{}, channels_{};

        std::unordered_map<GTextureSettings, Scope<GTexture>, GTextureSettingsHash>
            gpu_variants_{};
    };
}
