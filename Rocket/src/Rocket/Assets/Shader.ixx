module;

#include <unordered_map>
#include "rke_macros.h"

export module Shader;

import Types;
import HeapManager;
import String;
import Path;
import GShader;

export namespace rke
{
    struct RKE_API ShaderSettings
    {
        // future: preprocessor defines, optimization level, etc.
        bool operator==(const ShaderSettings&) const = default;
    };

    struct RKE_API ShaderSettingsHash
    {
        Size operator()(const ShaderSettings&) const { return 0ull; }
    };

    class RKE_API Shader
    {
    public:
        Shader(const Path& rkshdr_path);
        ~Shader() = default;

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = default;
        Shader& operator=(Shader&&) = default;

        const String& get_name() const { return name_; }

        // lazily compiles & caches a GPU program for the given settings
        GShader* get_gshader(const ShaderSettings& settings = {});
        inline void clear_gshaders() { gpu_variants_.clear(); }
    private:
        String name_{};
        ShaderPaths shader_paths_{};

        std::unordered_map<ShaderSettings, Scope<GShader>, ShaderSettingsHash>
            gpu_variants_{};
    };
}

namespace rke
{
    class ShaderHasher
    {
    public:
        ShaderHasher() = default;
        ~ShaderHasher() = default;

        uint64 hash(const Path& path, const String& source);
    private:
        // Hashes main source plus every (recursively) included file, so that a
        // change in either invalidates the SPIR-V cache. Depth cap guards cycles.
        void hash_source_recursive(uint64& hash,
            const Path& source_path, const String& source, uint32 depth);
    private: // static
        static uint64 fnv1a_update(uint64 hash, const void* data, Size size);
        static inline uint64 fnv1a_string(uint64 hash, const String& s)
            { return fnv1a_update(hash, s.raw(), s.length()); }

        // Returns [quote_begin, quote_end) of the first `#include "xxx"` on the
        // line [line_begin, line_end), or false when this line isn't an include.
        static bool try_parse_include_line(const String& source,
            Size line_begin, Size line_end, Size& out_begin, Size& out_end);
    private:
        // content fingerprint helpers (FNV-1a 64bit over source bytes)
        static constexpr uint64 fnv1a_offset_basis{ 0xcbf29ce484222325ull };
        static constexpr uint64 fnv1a_prime{ 0x100000001b3ull };

        std::unordered_map<Path, String> cached_sources_{};
    };
}
