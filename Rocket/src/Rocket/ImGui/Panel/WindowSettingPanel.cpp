module;
module WindowSettingPanel;

import Layout;
import DeltaTime;
import Window;

namespace rke
{
    void WindowSettingPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());

        layout::tree_node_branch<u8"V-Sync">([&]()
        {
            Window& owner{ *context_ };
            float panel_w{ ImGui::GetContentRegionAvail().x };
            ImGui::SetNextItemWidth(panel_w - 10.0f);
            if(ImGui::SliderFloat("##extent",
             &(owner.get_vsync_extent_mut()), 0.0f, 1.0f, "%.2f"))
                owner.update_vsync();
            bool is_vsync{ static_cast<bool>(owner.get_vsync_extent()) };
            if(ImGui::Checkbox("V-Sync On", &is_vsync)) {
                owner.get_vsync_extent_mut() = is_vsync ? 1.0f : 0.0f;
                owner.update_vsync();
            }
        });

        // TO REMOVE
        layout::tree_node_branch<u8"FPS">([&]()
            { ImGui::Text("Global: %d", DeltaTime::get_slow_fps()); });

        ImGui::End();
    }

    void WindowSettingPanel::set_context(Window* window) { context_ = window; }
}
