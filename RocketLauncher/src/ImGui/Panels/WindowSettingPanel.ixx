module;
export module WindowSettingPanel;

import rke;

import Layout;

export namespace rke
{
    class WindowSettingPanel : public Panel
    {
    public:
        WindowSettingPanel(String name) : Panel(std::move(name)) {}

        void set_context(Window* window) { context_ = window; }
        void on_imgui_render() override;
    private:
        Window* context_{};
    };

    void WindowSettingPanel::on_imgui_render()
    {
        ImGui::PushID(get_name().raw());
        ImGui::Begin (get_name().raw());

        layout::tree_node_branch(u8"V-Sync", [&]()
        {
            float panel_w{ ImGui::GetContentRegionAvail().x };
            ImGui::SetNextItemWidth(panel_w - 10.0f);
            if(ImGui::SliderFloat("##extent",
             &(context_->get_vsync_extent_mut()), 0.0f, 1.0f, "%.2f"))
                context_->update_vsync();
            bool is_vsync{ static_cast<bool>(context_->get_vsync_extent()) };
            if(ImGui::Checkbox("V-Sync On", &is_vsync)) {
                context_->get_vsync_extent_mut() = is_vsync ? 1.0f : 0.0f;
                context_->update_vsync();
            }
        });

        layout::tree_node_branch(u8"FPS", [&]() {
            ImGui::Text("Global: %d", DeltaTime::get_slow_fps()); // TEMP
        });

        ImGui::End();
        ImGui::PopID();
    }
}
