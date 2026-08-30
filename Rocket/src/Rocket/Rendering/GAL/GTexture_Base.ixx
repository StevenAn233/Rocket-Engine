module;

#include <array>
#include <functional>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module GTexture:Base;

import Types;
import Log;
import String;
import Path;
import HeapManager;
import BindingPoint;

export namespace rke
{
    class RKE_API GTexture
    {
    public:
        enum class Format
        {
            None = 0,
            R8, RGB8, RGBA8, RGBA16F,
            SRGB8, SRGB8_ALPHA8,
            R32I, DEPTH24_STENCIL8
        };
        enum class FiltFormat : uint32 { Linear	= 0, Nearest };
        enum class WrapFormat : uint32 { Clamp2Edge = 0, Repeat };

        GTexture() = default;
        virtual ~GTexture() = default;

        GTexture(const GTexture&) = delete;
        GTexture& operator=(const GTexture&) = delete;
        GTexture(GTexture&&) = delete;
        GTexture& operator=(GTexture&&) = delete;

        virtual uint32 get_width() const = 0;
        virtual uint32 get_height() const = 0;
        virtual uint32 get_channels() const = 0;
        virtual uint32 get_gal_id() const = 0;

        virtual void set_data(void* data, uint32 size) = 0;
        virtual void bind(BindingPoint point) const = 0;
        virtual void bind(uint32 slot) const = 0;

        [[nodiscard]] static constexpr StringView to_str(FiltFormat filt)
        {
            switch(filt)
            {
            case FiltFormat::Nearest: return u8"nearest";
            case FiltFormat::Linear:  return u8"linear";
            default: return u8"linear";
            }
        }

        [[nodiscard]] static constexpr StringView to_str(WrapFormat wrap)
        {
            switch(wrap)
            {
            case WrapFormat::Repeat:	 return u8"repeat";
            case WrapFormat::Clamp2Edge: return u8"clamp_to_edge";
            default: return u8"clamp_to_edge";
            }
        }
    };

    class RKE_API GTexture2D : public GTexture
    {
    public:
        GTexture2D() = default;
        ~GTexture2D() override = default;

        static Scope<GTexture2D> create(uint32 w, uint32 h,
            Format format, const void* data,
            FiltFormat filt, WrapFormat wrap);

        static Scope<GTexture2D> create_from_id(uint32 gal_id,
            uint32 w, uint32 h, Format format);
    };

    struct RKE_API GTextureSettings
    {
        GTexture::FiltFormat filt{ GTexture::FiltFormat::Linear };
        GTexture::WrapFormat wrap{ GTexture::WrapFormat::Clamp2Edge };
        bool srgb{ true };

        GTextureSettings() = default;
        GTextureSettings(GTexture::FiltFormat f, GTexture::WrapFormat w, bool s)
            : filt(f), wrap(w), srgb(s) {}

        bool operator==(const GTextureSettings&) const = default;
    };

    struct RKE_API GTextureSettingsHash
    {
        Size operator()(const GTextureSettings& s) const
        {
            Size h1{ std::hash<int>{}(static_cast<int>(s.filt)) };
            Size h2{ std::hash<int>{}(static_cast<int>(s.wrap)) };
            Size h3{ std::hash<bool>{}(s.srgb) };
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}
