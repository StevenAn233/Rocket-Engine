module;
module Panel;

namespace rke
{
    void Panel::refresh_state()
    {
        is_hovered_ = ImGui::IsWindowHovered();
        is_focused_ = ImGui::IsWindowFocused();
    }
}
