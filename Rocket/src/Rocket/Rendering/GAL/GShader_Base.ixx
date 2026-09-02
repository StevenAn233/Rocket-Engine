module;

#include <array>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module GShader:Base;

import Types;
import String;
import Path;
import HeapManager;

export namespace rke
{
    enum class ShaderStage : uint32
    {
        Vertex         = 0,
        Fragment       = 1,
        Geometry       = 2,
        Compute        = 3,
        TessControl    = 4,
        TessEvaluation = 5
    };
    
    using ShaderSources = std::array<String, 6>; // shader stages count
    using ShaderPaths = std::array<Path, 6>;

    class RKE_API GShader
    {
    public:
        GShader() = default;
        virtual ~GShader() = default;

        GShader(const GShader&) = delete;
        GShader& operator=(const GShader&) = delete;
        GShader(GShader&&) = delete;
        GShader& operator=(GShader&&) = delete;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;

        virtual const GShader& set_uniform(StringView name, int  slot) const = 0;
        virtual const GShader& set_uniform(StringView name, float val) const = 0;
        virtual const GShader& set_uniform(StringView name, int count, int* data) const = 0;
        virtual const GShader& set_uniform(StringView name, glm::vec2 vec) const = 0;
        virtual const GShader& set_uniform(StringView name, glm::vec3 vec) const = 0;
        virtual const GShader& set_uniform(StringView name, glm::vec4 vec) const = 0;

        virtual uint32 get_gal_id() const = 0;

        static Scope<GShader> create(const String& name, const ShaderPaths& paths, const ShaderSources& sources);
    };
}
