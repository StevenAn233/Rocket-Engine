module;

#include <imgui.h>

export module Style;

export namespace rke::style
{
    inline void imgui_darktheme()
    {
        ImGui::StyleColorsDark();

        auto& colors{ ImGui::GetStyle().Colors };
        colors[ImGuiCol_WindowBg] = ImVec4{ 0.05f, 0.0525f, 0.055f, 1.0f };

        // Headers
        colors[ImGuiCol_Header]	        = ImVec4{ 0.2f,  0.205f,  0.21f,  1.0f };
        colors[ImGuiCol_HeaderHovered]	= ImVec4{ 0.3f,  0.305f,  0.31f,  1.0f };
        colors[ImGuiCol_HeaderActive]	= ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Buttons
        colors[ImGuiCol_Button]	        = ImVec4{ 0.2f,  0.205f,  0.21f,  1.0f };
        colors[ImGuiCol_ButtonHovered]	= ImVec4{ 0.3f,  0.305f,  0.31f,  1.0f };
        colors[ImGuiCol_ButtonActive]	= ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Frame Background
        colors[ImGuiCol_FrameBg]	    = ImVec4{ 0.2f,  0.205f,  0.21f,  1.0f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f,  0.305f,  0.31f,  1.0f };
        colors[ImGuiCol_FrameBgActive]	= ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Tabs
        colors[ImGuiCol_Tab]                = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabHovered]	        = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
        colors[ImGuiCol_TabActive]	        = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
        colors[ImGuiCol_TabUnfocused]	    = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.20f, 0.2050f, 0.210f, 1.0f };

        // Title Background
        colors[ImGuiCol_TitleBg]	      = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgActive]	  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    }
}
