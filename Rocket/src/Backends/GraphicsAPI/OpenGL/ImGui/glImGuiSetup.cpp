module;

#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>

module ImGuiSetup;

namespace rke::imgui
{
    void disable_bind_sampler()
    {
        auto* render_state{ ImGui_ImplOpenGL3_GetRenderState() };
        render_state->UseBindSampler = false;
        glBindSampler(0, 0); 
    }
}

namespace rke::imgui::internal
{
    void init_graphics() { ImGui_ImplOpenGL3_Init("#version 430 core"); }

    void shutdown_graphics() { ImGui_ImplOpenGL3_Shutdown(); }
    
    void begin_render_graphics() { ImGui_ImplOpenGL3_NewFrame(); }

    void end_render_graphics()
        { ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); }
}
