module;

#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>

export module Shader:OpenGL;

import :Base;
import String;
import Path;

namespace rke
{
    using ShaderSources = std::unordered_map<GLenum, std::pair<Path, String>>;
    using ShaderPaths   = std::unordered_map<GLenum, Path>;
    using ShadersCache  = std::unordered_map<GLenum, std::vector<uint32>>;

    class glShader : public Shader
    {
    public:
        glShader(const String& name, const ShaderPathMap& paths);
        ~glShader() override;

        void bind  () const override;
        void unbind() const override;

        const String& get_name() const override { return name_; }
        uint32 get_renderer_id() const override { return renderer_id_; }

        const Shader& set_uniform(StringView name, int  slot) const override
            { return upload(name, slot); }
        const Shader& set_uniform(StringView name, float val) const override
            { return upload(name, val); }
        const Shader& set_uniform(StringView name, int count, int* data) const override
            { return upload(name, count, data); }
        const Shader& set_uniform(StringView name, glm::vec2 vec) const override
            { return upload(name, vec); }
        const Shader& set_uniform(StringView name, glm::vec3 vec) const override
            { return upload(name, vec); }
        const Shader& set_uniform(StringView name, glm::vec4 vec) const override
            { return upload(name, vec); }

        const Shader& upload(StringView name,  int  v) const;
        const Shader& upload(StringView name, float v) const;
        const Shader& upload(StringView name, int count, int* data) const;
        const Shader& upload(StringView name, float v0, float v1, float v2) const;
        const Shader& upload(StringView name, glm::vec2 vec) const;
        const Shader& upload(StringView name, glm::vec3 vec) const;
        const Shader& upload(StringView name, glm::vec4 vec) const;
        const Shader& upload(StringView name,
                             float v0, float v1, float v2, float v3) const;
    private:
        void compile_or_get_vulkan_spirv(const ShaderSources& sources);
        void compile_or_get_opengl_spirv(const ShaderPaths  & paths  );
        void create_program();

        int get_uniform_location(const String& name) const; // Can't be StringView!!!
    private:
        uint32 renderer_id_{};
        String name_{};

        mutable std::unordered_map<String, int> uniform_location_cache_{};

        ShadersCache vulkan_spirv_{};
        ShadersCache opengl_spirv_{};
    };
}
