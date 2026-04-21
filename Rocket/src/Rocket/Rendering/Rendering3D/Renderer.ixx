module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module Renderer;

import HeapManager;
import VertexArray;
import Shader;
import RenderCommand;

export namespace rke
{
    class RKE_API Renderer
    {
    public:
        static void init();
        static void shutdown();

        static void begin_scene() {} // 3D rendering
        static void end_scene  () {} // 3D rendering

        static void submit (
            const Ref<Shader>& shader, const Ref<VertexArray>& vao,
            const glm::mat4& mod_mat = glm::mat4(1.0f)
        );
    };
}
