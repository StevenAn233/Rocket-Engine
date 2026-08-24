module;
module ApplicationPanel;

import Layout;
import Instrumentor;
import Renderer2D;
import Application;

namespace rke
{
    void ApplicationPanel::on_imgui_render()
    {
        RKE_PROFILE_FUNCTION();

        ImGui::Begin("Application");

    #if RKE_ENABLE_PROFILE
        layout::tree_node_branch<u8"Instrumentation">([&]()
        {
            if(app().instrumentor().is_session_running())
            {
                if(ImGui::SmallButton("Stop"))
                {
                    if(app().instrumentor().is_session_running())
                        RKE_PROFILE_END_SESSION();
                }
                ImGui::SameLine();

                ImGui::Text("Status:");
                ImGui::SameLine();
                // show green "ACTIVE"
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ACTIVE");

                ImGui::Text("Duration: %.2f s", app().instrumentor().get_session_duration_s());
            } else {
                if(ImGui::SmallButton("Begin"))
                {
                    RKE_PROFILE_BEGIN_SESSION
                    (u8"Manual Profile Session", u8"profile_session.json");
                }
                ImGui::SameLine();

                ImGui::Text("Status:");
                ImGui::SameLine();
                // show red "INACTIVE"
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "INACTIVE");
            }
        });
    #endif // RKE_ENABLE_PROFILE

        ImGui::End();
    }
}
