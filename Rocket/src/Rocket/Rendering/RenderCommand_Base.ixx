module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module RenderCommand:Base;

import Types;
import Log;
import String;
import Instrumentor;
import HeapManager;
import VertexArray;

export namespace rke::debug {
    void check_shader(uint32 expected_program_id, const String& shader_name);
}

export namespace rke
{
    class RKE_API RenderCommand
    {
    public:
        friend struct std::default_delete<RenderCommand>;

        RenderCommand(const RenderCommand&) = delete;
        RenderCommand& operator=(const RenderCommand&) = delete;
        RenderCommand(RenderCommand&&) = delete;
        RenderCommand& operator=(RenderCommand&&) = delete;

        static void enable_blend () { get_instance().enable_blend_impl (); }
        static void disable_blend() { get_instance().disable_blend_impl(); }
        static void blend_func_default	  () { get_instance().blend_func_default_impl	 (); }
        static void blend_func_transparent() { get_instance().blend_func_transparent_impl(); }

        static void enable_srgb () { get_instance().enable_srgb_impl (); }
        static void disable_srgb() { get_instance().disable_srgb_impl(); }

        static void enable_depth_test() { get_instance().enable_depth_test_impl(); }
        static void set_depth_write(bool enabled)
            { get_instance().set_depth_write_impl(enabled); }
        static void disable_depth_test() { get_instance().disable_depth_test_impl(); }

        static void set_viewport(uint32 x, uint32 y, uint32 w, uint32 h)
            { get_instance().set_viewport_impl(x, y, w, h); }
        static void clear_color_buffer(uint32 fbo, int color_attach_index, int val)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().clear_color_buffer_impl(fbo, color_attach_index, val);
        }
        static void clear_color_buffer(uint32 fbo, int color_attach_index, float val)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().clear_color_buffer_impl(fbo, color_attach_index, val);
        }
        static void clear_color_buffer(uint32 fbo, int color_attach_index, glm::vec3 val)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().clear_color_buffer_impl(fbo, color_attach_index, val);
        }
        static void clear_color_buffer(uint32 fbo, int color_attach_index, glm::vec4 val)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().clear_color_buffer_impl(fbo, color_attach_index, val);
        }
        static void clear_depth_buffer(uint32 fbo, float depth = 1.0f, int stencil = 0)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().clear_depth_buffer_impl(fbo, depth, stencil);
        }

        static void draw(int start, int end)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().draw_impl(start, end);
        }
        static void draw_quad() { draw(0, 4); }
        static void draw_indexed(const Ref<VertexArray>& vao)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().draw_indexed_impl(vao);
        }
        static void draw_indexed(int count)
        {
            RKE_PROFILE_FUNCTION();
            get_instance().draw_indexed_impl(count);
        }
    protected:
        RenderCommand() = default;
        virtual ~RenderCommand() = default;

        virtual void enable_blend_impl () = 0;
        virtual void disable_blend_impl() = 0;
        virtual void blend_func_default_impl() = 0;
        virtual void blend_func_transparent_impl() = 0;

        virtual void set_depth_write_impl(bool enabled) = 0;
        virtual void enable_depth_test_impl () = 0;
        virtual void disable_depth_test_impl() = 0;

        virtual void enable_srgb_impl () = 0;
        virtual void disable_srgb_impl() = 0;

        virtual void set_viewport_impl(uint32 x, uint32 y, uint32 w, uint32 h) = 0;
        virtual void clear_color_buffer_impl(uint32 fbo, int color_attach_index,  int  val) = 0;
        virtual void clear_color_buffer_impl(uint32 fbo, int color_attach_index, float val) = 0;
        virtual void clear_color_buffer_impl(uint32 fbo, int color_attach_index, glm::vec3 val) = 0;
        virtual void clear_color_buffer_impl(uint32 fbo, int color_attach_index, glm::vec4 val) = 0;
        virtual void clear_depth_buffer_impl(uint32 fbo, float depth, int stencil) = 0;

        virtual void draw_impl(int start, int end) = 0;
        virtual void draw_indexed_impl(const Ref<VertexArray>& vao) = 0;
        virtual void draw_indexed_impl(int count) = 0;
    private:
        static RenderCommand& get_instance();
        static Scope<RenderCommand> create();
    };
}
