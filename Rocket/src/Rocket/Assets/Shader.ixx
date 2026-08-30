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
    using ShaderPathMap = std::unordered_map<ShaderStage, Path>;

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
        Shader(const Path& shader_path);
        ~Shader() = default;

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = default;
        Shader& operator=(Shader&&) = default;

        const String& get_name() const { return name_; }

        // lazily compiles & caches a GPU program for the given settings
        GShader* get_gshader(const ShaderSettings& settings = {});
    private:
        String name_{};
        ShaderPathMap stage_paths_{};

        std::unordered_map<ShaderSettings, Scope<GShader>, ShaderSettingsHash>
            gpu_variants_{};
    };
}
