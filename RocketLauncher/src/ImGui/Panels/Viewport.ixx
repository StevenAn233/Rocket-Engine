module;
export module Viewport;

import rke;

export namespace rke
{
    class Viewport : public Panel
    {
    public:
        Viewport(String name) : Panel(std::move(name)) {}

        void on_imgui_render() override;

        void set_in_viewport_callback(std::function<void(Viewport*)> callback);
        void set_next_render_target(uint32 render_target_id)
            { next_render_target_id_ = render_target_id; }

        glm::vec2 get_viewport_size()
        {
            if(!enabled()) return glm::vec2(0.0f);
            return glm::vec2(viewport_size_.x, viewport_size_.y);
        }
        glm::vec2 get_viewport_mouse()
        {
            if(!enabled()) return glm::vec2(0.0f);
            return glm::vec2(viewport_mouse_.x, viewport_mouse_.y);
        }

        bool is_hovered() const { return is_hovered_; }
        bool is_focused() const { return is_focused_; }
    private:
        uint32 next_render_target_id_{};

        ImVec2 viewport_size_ { 0.0f, 0.0f };
        ImVec2 viewport_mouse_{ 0.0f, 0.0f };

        bool is_hovered_{ false };
        bool is_focused_{ false };

        std::function<void(Viewport*)> in_viewport_callback_{};
    };
}
