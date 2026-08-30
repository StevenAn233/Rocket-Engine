module;

#include "rke_macros.h"

export module Font;

import Types;
import GTexture;
import FileUtils;
import HeapManager;
import String;
import Path;

export namespace rke
{
    class RKE_API Font
    {
    public:
        Font(const Path& path, float font_size = 64.0f, uint32 sampling = 2u);
        Font(const Font&) = delete;
        Font(Font&&) = default;
        ~Font();

        GTexture2D* get_font_atlas() const { return font_atlas_.get(); }
        const void* get_char_data() const { return char_data_; }

        uint32 get_sampling() const { return sampling_; }
        float get_font_size() const { return font_size_; }
        float get_atlas_size() const { return atlas_size_; }
    private:
        float font_size_;
        uint32 sampling_;
        uint32 atlas_size_;

        Scope<GTexture2D> font_atlas_{};
        void* char_data_{}; // stbtt_packedchar*
    };
}
