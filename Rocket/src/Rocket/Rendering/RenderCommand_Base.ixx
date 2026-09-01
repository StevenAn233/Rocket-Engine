module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module RenderCommand:Base;

import Types;
import Log;
import String;
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
        RenderCommand() = default;
        virtual ~RenderCommand() = default;

        RenderCommand(const RenderCommand&) = delete;
        RenderCommand& operator=(const RenderCommand&) = delete;
        RenderCommand(RenderCommand&&) = delete;
        RenderCommand& operator=(RenderCommand&&) = delete;
    
        virtual void enable_blend() = 0;
        virtual void disable_blend() = 0;
        virtual void blend_func_default() = 0;
        virtual void blend_func_useless() = 0;

        virtual void set_depth_write(bool enabled) = 0;
        virtual void enable_depth_test() = 0;
        virtual void disable_depth_test() = 0;

        virtual void enable_srgb() = 0;
        virtual void disable_srgb() = 0;

        virtual void set_viewport(uint32 x, uint32 y, uint32 w, uint32 h) = 0;
        virtual void clear_color_buffer(uint32 fbo, int color_attach_index,  int  val) = 0;
        virtual void clear_color_buffer(uint32 fbo, int color_attach_index, float val) = 0;
        virtual void clear_color_buffer(uint32 fbo, int color_attach_index, glm::vec3 val) = 0;
        virtual void clear_color_buffer(uint32 fbo, int color_attach_index, glm::vec4 val) = 0;
        virtual void clear_depth_buffer(uint32 fbo, float depth, int stencil) = 0;

        virtual void draw(int start, int end) = 0;
        virtual void draw_indexed(const VertexArray& vao) = 0;
        virtual void draw_indexed(int count) = 0;
        virtual void draw_instanced(int index_count, int instance_count, int base_instance) = 0;
        
        inline void draw_quad() { draw(0, 4); }
    
        static Scope<RenderCommand> create();
    };
}
