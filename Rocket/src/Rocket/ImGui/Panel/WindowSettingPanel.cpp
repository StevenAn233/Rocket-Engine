module;
module WindowSettingPanel;

import Log;
import Layout;
import Window;

namespace rke
{
    WindowSettingPanel::WindowSettingPanel(String name, Window* owner)
        : Panel(String::format(u8"Window: '{}'", std::move(name))), owner_(owner)
        { CORE_ASSERT(owner_, u8"WindowSettingPanel: Owner window null!"); }

    void WindowSettingPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());
        if(!ImGui::BeginTabBar(get_name().raw())) return;

        if(ImGui::BeginTabItem("Settings"))
        {
            layout::tree_node_branch<u8"V-Sync">([&]()
            {
                float panel_w{ ImGui::GetContentRegionAvail().x };
                ImGui::SetNextItemWidth(panel_w - 10.0f);
                if(ImGui::SliderFloat("##extent",
                &(owner_->get_vsync_extent_mut()), 0.0f, 1.0f, "%.2f"))
                    owner_->update_vsync();
                bool is_vsync{ static_cast<bool>(owner_->get_vsync_extent()) };
                if(ImGui::Checkbox("V-Sync On", &is_vsync)) {
                    owner_->get_vsync_extent_mut() = is_vsync ? 1.0f : 0.0f;
                    owner_->update_vsync();
                }
            });

            layout::tree_node_branch<u8"FPS">([&]()
                { ImGui::Text("Smoothed: %.2f", owner_->get_smoothed_fps()); });

            ImGui::EndTabItem();
        }

    #ifdef RKE_ENABLE_STATISTICS
        if(ImGui::BeginTabItem("Statistics"))
        {
            const auto& stats{ owner_->renderer().get_stats() };
            ImGui::Text("CamSets  : %d", stats.cam_set_count );
            ImGui::Text("DrawCalls: %d", stats.drawcall_count);
            ImGui::Text("Instances: %d", stats.instance_count);

            ImGui::EndTabItem();
        }
    #endif // RKE_ENABLE_STATISTICS

        ImGui::EndTabBar();
        ImGui::End();
    }
}
