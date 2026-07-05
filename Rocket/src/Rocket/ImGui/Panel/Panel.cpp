module;
module Panel;

namespace rke
{
    void Panel::refresh_state()
    {
        is_hovered_ = ImGui::IsWindowHovered();
        is_focused_ = ImGui::IsWindowFocused();

        ImVec2 this_size{ ImGui::GetContentRegionAvail() };
        if(this_size.x != size_.x || this_size.y != size_.y) resized_ = true;
        else resized_ = false;
        size_ = std::bit_cast<glm::vec2>(this_size);

        ImVec2 this_abs_pos{ ImGui::GetWindowPos() };
        if(this_abs_pos.x != abs_pos_.x || this_abs_pos.y != abs_pos_.y) relocated_ = true;
        else relocated_ = false;
        abs_pos_ = std::bit_cast<glm::vec2>(this_abs_pos);

        abs_mouse_pos_ = std::bit_cast<glm::vec2>(ImGui::GetMousePos());
        mouse_pos_.x = abs_mouse_pos_.x - abs_pos_.x;
        mouse_pos_.y = size_.y - (abs_mouse_pos_.y - abs_pos_.y);
    }
}
