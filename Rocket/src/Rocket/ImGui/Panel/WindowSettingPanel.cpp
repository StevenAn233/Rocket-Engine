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

        update_smoothed_fps();
        layout::tree_node_branch<u8"FPS">([&]()
            { ImGui::Text("Smoothed: %.2f", smoothed_fps_); });

        ImGui::End();
    }

    void WindowSettingPanel::update_smoothed_fps()
    {
        constexpr float alpha{ 5.0f };

        double dt{ owner_->get_last_elapsed() };
        double current_fps{ 1.0 / dt };

        double lerp_alpha{ glm::clamp(dt * alpha, 0.0, 1.0) };
        smoothed_fps_ = glm::mix(smoothed_fps_, current_fps, lerp_alpha);
    }
}
