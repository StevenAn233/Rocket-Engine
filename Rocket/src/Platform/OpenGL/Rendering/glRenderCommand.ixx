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
    protected:
        void enable_blend_impl () override;
        void disable_blend_impl() override;
        void blend_func_default_impl	() override;
        void blend_func_transparent_impl() override;
        void enable_srgb_impl () override;
        void set_depth_write_impl(bool enabled) override;
        void enable_depth_test_impl () override;
        void disable_srgb_impl() override;
        void disable_depth_test_impl() override;
        void set_viewport_impl(uint32 x, uint32 y,
                               uint32 w, uint32 h) override;
        void clear_color_buffer_impl(uint32 fbo, int color_attach_index,  int  val) override;
        void clear_color_buffer_impl(uint32 fbo, int color_attach_index, float val) override;
        void clear_color_buffer_impl(uint32 fbo, int color_attach_index, glm::vec3 val) override;
        void clear_color_buffer_impl(uint32 fbo, int color_attach_index, glm::vec4 val) override;
        void clear_depth_buffer_impl(uint32 fbo, float depth, int stencil) override;

        void draw_impl(int start, int end) override;
        void draw_indexed_impl(const Ref<VertexArray>& vao) override;
        void draw_indexed_impl(int count) override;
    };
}
