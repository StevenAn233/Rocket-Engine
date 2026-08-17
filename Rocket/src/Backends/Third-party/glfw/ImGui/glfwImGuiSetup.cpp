module;

#include <backends/imgui_impl_glfw.h>
#include <glfw/glfw3.h>

module ImGuiSetup;

namespace rke::imgui::internal
{
    void init_window(NativeWindow context)
    {
        ImGui_ImplGlfw_InitForOpenGL
            (reinterpret_cast<GLFWwindow*>(context.get()), true);
        // TO MODIFY
    }

    void shutdown_window() { ImGui_ImplGlfw_Shutdown(); }

    void begin_render_window() { ImGui_ImplGlfw_NewFrame(); }

    void end_render_window()
    {
        if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}
