module;

#include <array>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Texture:Base;

import Types;
import Log;
import String;
import Path;
import HeapManager;
import BindingPoint;
import MathUtils;

export namespace rke
{
    class RKE_API Texture
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

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = delete;
        Texture& operator=(Texture&&) = delete;

        virtual uint32 get_width () const = 0;
        virtual uint32 get_height() const = 0;

        virtual uint32 get_channels   () const = 0;
        virtual uint32 get_renderer_id() const = 0;

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

        inline std::array<glm::vec2, 4> calc_uv(glm::vec2 coords,
            glm::vec2 cell_size, glm::vec2 cells = { 1, 1 }) const
        {
            glm::vec2 min {
                (coords.x * cell_size.x) / get_width(),
                (coords.y * cell_size.y) / get_height()
            };
            glm::vec2 max {
                ((coords.x + cells.x) * cell_size.x) / get_width(),
                ((coords.y + cells.y) * cell_size.y) / get_height()
            };
            return math::calc_uv(min, max);
        }
    protected:
        Texture() = default;
        virtual ~Texture() = default;
    };

    class RKE_API Texture2D : public Texture
    {
    public:
        static Ref<Texture2D> create(uint32 w, uint32 h,
            Format format, FiltFormat filt = FiltFormat::Linear);
        static Ref<Texture2D> create(const Path& filepath,
            FiltFormat filt = FiltFormat::Linear,
            WrapFormat wrap = WrapFormat::Clamp2Edge, bool srgb = true);
        static Ref<Texture2D> create_from_id
            (uint32 renderer_id, uint32 w, uint32 h, Format format);
    protected:
        Texture2D() = default;
        ~Texture2D() override = default;
    };
}
