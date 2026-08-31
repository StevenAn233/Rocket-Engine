module;

#include <glm/glm.hpp>

export module RenderCommand:OpenGL;

import :Base;
import Types;
import HeapManager;

namespace rke
{
    class glRenderCommand : public RenderCommand
    {
    public:
        glRenderCommand() = default;
        ~glRenderCommand() override = default;
    
        void enable_blend() override;
        void disable_blend() override;
        void blend_func_default() override;
        void blend_func_transparent() override;
        void enable_srgb() override;
        void set_depth_write(bool enabled) override;
        void enable_depth_test() override;
        void disable_srgb()override;
        void disable_depth_test() override;
        void set_viewport(uint32 x, uint32 y, uint32 w, uint32 h) override;
        void clear_color_buffer(uint32 fbo, int color_attach_index,  int  val) override;
        void clear_color_buffer(uint32 fbo, int color_attach_index, float val) override;
        void clear_color_buffer(uint32 fbo, int color_attach_index, glm::vec3 val) override;
        void clear_color_buffer(uint32 fbo, int color_attach_index, glm::vec4 val) override;
        void clear_depth_buffer(uint32 fbo, float depth, int stencil) override;

        void draw(int start, int end) override;
        void draw_indexed(const VertexArray& vao) override;
        void draw_indexed(int count) override;
        void draw_instanced(int index_count, int instance_count, int base_instance) override;
    };
}
