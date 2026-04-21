module;

#include <glad/glad.h>

module RenderCommand;
import :OpenGL;

import Log;
import String;

namespace rke::debug
{
    void check_shader(uint32 expected_program_id, const String& shader_name)
    {
        GLint current_program{};
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);

        // --- 1: is current id correct? ---
        if((GLuint)current_program != expected_program_id)
        {
            CORE_ERROR(u8"SHADER_VALIDATION FAILED for '{0}':", shader_name);
            CORE_ERROR(u8"  - Reason: Program ID mismatch!");
            CORE_ERROR(u8"  - Expected: {0}, Currently Bound: {1}", expected_program_id, current_program);
            if(current_program == 0) CORE_ERROR(u8"  - Diagnosis: No shader program is currently bound!");
            return;
        }

        // --- 2: is id valid? ---
        if(!glIsProgram(expected_program_id))
        {
            CORE_ERROR(u8"SHADER_VALIDATION FAILED for '{0}' (ID: {1}):", shader_name, expected_program_id);
            CORE_ERROR(u8"  - Reason: The ID does not correspond to a valid program object.");
            CORE_ERROR(u8"  - Diagnosis: The shader object may have been deleted or was never created correctly.");
            return;
        }

        // --- 3: is program linked? ---
        GLint link_status = 0;
        glGetProgramiv(expected_program_id, GL_LINK_STATUS, &link_status);
        if(link_status == GL_FALSE)
        {
            GLint log_length = 0;
            glGetProgramiv(expected_program_id, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<GLchar> info_log(log_length);
            glGetProgramInfoLog(expected_program_id, log_length, nullptr, info_log.data());

            CORE_ERROR(u8"SHADER_VALIDATION FAILED for '{0}' (ID: {1}):", shader_name, expected_program_id);
            CORE_ERROR(u8"  - Reason: Program failed to link!");
            CORE_ERROR(u8"  - Linker Log:\n{0}", info_log.data());
            return;
        }

        // --- 4: check opengl ---
        glValidateProgram(expected_program_id);
        GLint validate_status = 0;
        glGetProgramiv(expected_program_id, GL_VALIDATE_STATUS, &validate_status);
        if(validate_status == GL_FALSE)
        {
            GLint log_length = 0;
            glGetProgramiv(expected_program_id, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<GLchar> info_log(log_length);
            glGetProgramInfoLog(expected_program_id, log_length, nullptr, info_log.data());

            CORE_ERROR(u8"SHADER_VALIDATION FAILED for '{0}' (ID: {1}):", shader_name, expected_program_id);
            CORE_ERROR(u8"  - Reason: Program is NOT VALID in the current OpenGL state!");
            CORE_ERROR(u8"  - Validation Log:\n{0}", info_log.data());
            return;
        }
    //  CORE_INFO("SHADER_VALIDATION PASSED for '{0}' (ID: {1})", shader_name, expected_program_id);
    }
}

namespace rke
{
    void glRenderCommand::enable_blend_impl()
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void glRenderCommand::disable_blend_impl() { glDisable(GL_BLEND); }

    void glRenderCommand::blend_func_default_impl()
        { glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }

    void glRenderCommand::blend_func_transparent_impl()
        { glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); }

    void glRenderCommand::enable_srgb_impl() { glEnable(GL_FRAMEBUFFER_SRGB); }

    void glRenderCommand::set_depth_write_impl(bool enabled)
    {
        if(enabled) glDepthMask(GL_TRUE);
        else glDepthMask(GL_FALSE);
    }

    void glRenderCommand::enable_depth_test_impl()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    void glRenderCommand::disable_srgb_impl() { glDisable(GL_FRAMEBUFFER_SRGB); }

    void glRenderCommand::disable_depth_test_impl()
    {
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
    }

    void glRenderCommand::set_viewport_impl(uint32 x, uint32 y, uint32 w, uint32 h)
    {
        glViewport(static_cast<int>(x), static_cast<int>(y),
                   static_cast<int>(w), static_cast<int>(h));
    }

    void glRenderCommand::clear_color_buffer_impl(uint32 fbo, int color_attach_index, int val)
    {
        const int values[]{ val, 0, 0, 1 };
        glClearNamedFramebufferiv(fbo, GL_COLOR, color_attach_index, values);
    }

    void glRenderCommand::clear_color_buffer_impl(uint32 fbo, int color_attach_index, float val)
    {
        const float values[]{ val, 0.0f, 0.0f, 1.0f };
        glClearNamedFramebufferfv(fbo, GL_COLOR, color_attach_index, values);
    }

    void glRenderCommand::clear_color_buffer_impl(uint32 fbo, int color_attach_index, glm::vec3 val)
    {
        const float values[]{ val.r, val.g, val.b, 1.0f };
        glClearNamedFramebufferfv(fbo, GL_COLOR, color_attach_index, values);
    }

    void glRenderCommand::clear_color_buffer_impl(uint32 fbo, int color_attach_index, glm::vec4 val)
    {
        const float values[]{ val.r, val.g, val.b, val.a };
        glClearNamedFramebufferfv(fbo, GL_COLOR, color_attach_index, values);
    }

    void glRenderCommand::clear_depth_buffer_impl(uint32 fbo, float depth, int stencil)
        { glClearNamedFramebufferfi(fbo, GL_DEPTH_STENCIL, 0, depth, stencil); }

    void glRenderCommand::draw_impl(int start, int end)
        { glDrawArrays(GL_TRIANGLE_STRIP, start, end); }

    void glRenderCommand::draw_indexed_impl(const Ref<VertexArray>& vao)
    {
    #ifdef RKE_DEBUG
        GLint is_attrib_enabled{};
        glGetVertexArrayIndexediv(vao->get(), 0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &is_attrib_enabled);
        if(!is_attrib_enabled)
            CORE_WARN(u8"glRenderCommand: Vertex Attribute 0 is NOT enabled!");
    #endif // RKE_DEBUG

        glDrawElements(GL_TRIANGLES, vao->get_ibo()->get_count(), GL_UNSIGNED_INT, nullptr);
    }

    void glRenderCommand::draw_indexed_impl(int count)
        { glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr); }
}
