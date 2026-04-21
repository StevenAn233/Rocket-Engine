module;

#include <unordered_map>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Shader:Base;

import Types;
import HeapManager;
import Log;
import String;
import Path;

export namespace rke
{
    enum class ShaderStage : uint32
    {
        Vertex = 0,
        Fragment,
        Geometry,
        Compute,
        TessControl,
        TessEvaluation
    };

    using ShaderPathMap = std::unordered_map<ShaderStage, Path>;

    class RKE_API Shader
    {
    public:
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = delete;
        Shader& operator=(Shader&&) = delete;

        virtual void bind  () const = 0;
        virtual void unbind() const = 0;

        virtual const Shader& set_uniform(StringView name, int  slot) const = 0;
        virtual const Shader& set_uniform(StringView name, float val) const = 0;
        virtual const Shader& set_uniform(StringView name, int count, int* data) const = 0;
        virtual const Shader& set_uniform(StringView name, glm::vec2 vec) const = 0;
        virtual const Shader& set_uniform(StringView name, glm::vec3 vec) const = 0;
        virtual const Shader& set_uniform(StringView name, glm::vec4 vec) const = 0;

        virtual const String& get_name() const = 0;
        virtual uint32 get_renderer_id() const = 0;

        static Ref<Shader> create(const Path& shader_path);
    protected:
        Shader() = default;
        virtual ~Shader() = default;
    };
}
