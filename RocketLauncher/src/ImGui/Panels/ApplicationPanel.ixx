module;
export module ApplicationPanel;

import rke;

export namespace rke
{
    class ApplicationPanel : public Panel
    {
    public:
        ApplicationPanel(String name) : Panel(std::move(name)) {}

        void on_imgui_render() override;
    };

    void ApplicationPanel::on_imgui_render()
    {
        RKE_PROFILE_FUNCTION();

        ImGui::Begin("Application");
        bool opened{ ImGui::BeginTabBar("##application_tabbar") };
        if(!opened) return;

    #ifdef RKE_ENABLE_STATISTICS
        if(ImGui::BeginTabItem("Statistics"))
        {
            ImGui::Text("CamSets  : %d", Renderer2D::get_stats().cam_set_count );
            ImGui::Text("DrawCalls: %d", Renderer2D::get_stats().drawcall_count);
            ImGui::Text("Quads    : %d", Renderer2D::get_stats().quad_count    );
            ImGui::Text("Vertices : %d", Renderer2D::get_stats().vertex_count());
            ImGui::Text("Indices  : %d", Renderer2D::get_stats().index_count ());

            ImGui::EndTabItem();
        }
    #endif // RKE_ENABLE_STATISTICS
    #if RKE_ENABLE_PROFILE
        if(ImGui::BeginTabItem("Profiler Controls"))
        {
            if(Instrumentor::get().is_session_running()) {
                if(ImGui::SmallButton("Stop"))
                {
                    if(Instrumentor::get().is_session_running())
                        RKE_PROFILE_END_SESSION();
                }
                ImGui::SameLine();

                ImGui::Text("Status:");
                ImGui::SameLine();
                // show green "ACTIVE"
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ACTIVE");

                ImGui::Text("Duration: %.2f s",
                    Instrumentor::get().get_session_duration_s()
                );
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
            ImGui::EndTabItem();
        }
    #endif // RKE_ENABLE_PROFILE

        ImGui::EndTabBar();
        ImGui::End();
    }
}
