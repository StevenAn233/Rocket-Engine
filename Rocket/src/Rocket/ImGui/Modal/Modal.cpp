module;
module Modal;

namespace rke
{
    void Modal::popup()
        { ImGui::OpenPopup(get_title().raw()); }
}
