module;
module LogPanel;

namespace rke
{
    void LogPanel::push_text(String text)
    {
    }

    void LogPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());

        ImGui::End();
    }
}
