module;
export module DockSpace;

import rke;

export namespace rke
{
    class DockSpace
    {
    public:
        DockSpace(String name) : name_(std::move(name)) {}
        ~DockSpace();

        void load_from(Path path);
        void set_menubar_callback(std::function<void()> callback)
            { menubar_callback_ = std::move(callback); }

        void on_imgui_render(glm::vec2 offset = { 0.0f, 0.0f },
                             glm::vec2 scale  = { 1.0f, 1.0f });
        // based on main_viewport
    private:
        String name_{};
        Path filepath_{};
        std::function<void()> menubar_callback_{};
        ImGuiDockNodeFlags dockspace_flags_{ ImGuiDockNodeFlags_None };
    };
}
