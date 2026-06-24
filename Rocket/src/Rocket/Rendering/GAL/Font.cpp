module;

#include <stb_truetype.h>

module Font;

import Log;

namespace rke
{
    Font::Font(const Path& path, float font_size, uint32 sampling)
        : font_size_ (font_size), sampling_(sampling)
        , atlas_size_(static_cast<uint32>(font_size_ * sampling_) * 10u)
    { // we have 95 chars here

        Buffer ttf{ file::read_file_binary(path) };
        byte* temp_bitmap{ new byte[atlas_size_ * atlas_size_]{} };

        // only standard ASCII
        char_data_ = new stbtt_packedchar[96]{}; // 127 - 32 = 95; + 1 for safety

    // stbtt stuffs
        stbtt_pack_context pack_context{};
        stbtt_PackBegin(&pack_context, temp_bitmap, atlas_size_, atlas_size_, 0, 1, nullptr);
        // parameters:
        // stbtt_pack_context* spc, unsigned char* pixels,
        // int pw, int ph, int stride_in_bytes, int padding,
        // void* alloc_context

        stbtt_PackSetOversampling(&pack_context, sampling_, sampling_); // over sampling for better quality

        stbtt_PackFontRange(&pack_context, ttf.data(),
            0, font_size_, 32, 95, (stbtt_packedchar*)char_data_);

        // parameters:
        // stbtt_pack_context* spc, const unsigned char* fontdata,
        // int font_index, float font_size,
        // int first_unicode_codepoint_in_range, int num_chars_in_range,
        // stbtt_packedchar* chardata_for_range

        stbtt_PackEnd(&pack_context);

    // create texture
        font_atlas_ = Texture2D::create(atlas_size_, atlas_size_, Texture::Format::R8);
        font_atlas_->set_data(temp_bitmap, atlas_size_ * atlas_size_);

        delete[] temp_bitmap;
    }
}
