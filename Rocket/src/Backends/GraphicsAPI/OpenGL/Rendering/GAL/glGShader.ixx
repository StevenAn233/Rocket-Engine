module;

#include <array>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>

export module GShader:OpenGL;

import :Base;
import String;
import Path;

namespace rke
{
    using ShaderIRCache = std::array<std::vector<uint32>, 6>;

    class glGShader : public GShader
    {
    public:
        glGShader(String name, const ShaderPaths& paths, const ShaderSources& sources);
        ~glGShader() override;

        void bind() const override;
        void unbind() const override;

        uint32 get_gal_id() const override { return gal_id_; }

        const GShader& set_uniform(StringView name, int  slot) const override
            { return upload(name, slot); }
        const GShader& set_uniform(StringView name, float val) const override
            { return upload(name, val); }
        const GShader& set_uniform(StringView name, int count, int* data) const override
            { return upload(name, count, data); }
        const GShader& set_uniform(StringView name, glm::vec2 vec) const override
            { return upload(name, vec); }
        const GShader& set_uniform(StringView name, glm::vec3 vec) const override
            { return upload(name, vec); }
        const GShader& set_uniform(StringView name, glm::vec4 vec) const override
            { return upload(name, vec); }

        const GShader& upload(StringView name,  int  v) const;
        const GShader& upload(StringView name, float v) const;
        const GShader& upload(StringView name, int count, int* data) const;
        const GShader& upload(StringView name, float v0, float v1, float v2) const;
        const GShader& upload(StringView name, glm::vec2 vec) const;
        const GShader& upload(StringView name, glm::vec3 vec) const;
        const GShader& upload(StringView name, glm::vec4 vec) const;
        const GShader& upload(StringView name, float v0, float v1, float v2, float v3) const;
    private:
        void compile_or_get_vulkan_spirv(const ShaderPaths& paths, const ShaderSources& sources);
        void compile_or_get_opengl_spirv(const ShaderPaths& paths, const ShaderSources& sources);
        void create_program();

        int get_uniform_location(const String& name) const; // Can't be StringView!!!
    private:
        uint32 gal_id_{};
        String name_{};

        mutable std::unordered_map<String, int> uniform_location_cache_{};

        ShaderIRCache vulkan_spirv_{ std::vector<uint32>{} };
        ShaderIRCache opengl_spirv_{ std::vector<uint32>{} };
    };
}
