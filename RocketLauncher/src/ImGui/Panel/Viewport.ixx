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

        inline void set_in_viewport_callback(std::function<void(Viewport*)> callback)
            { in_viewport_callback_ = std::move(callback); }
        inline void set_render_target(uint32 id) { render_target_id_ = id; }

        inline glm::vec2 get_viewport_size()
        {
            if(!on()) return glm::vec2(0.0f);
            return glm::vec2(viewport_size_.x, viewport_size_.y);
        }

        inline glm::vec2 get_viewport_mouse()
        {
            if(!on()) return glm::vec2(0.0f);
            return glm::vec2(viewport_mouse_.x, viewport_mouse_.y);
        }
    private:
        uint32 render_target_id_{};

        ImVec2 viewport_size_ { 0.0f, 0.0f };
        ImVec2 viewport_mouse_{ 0.0f, 0.0f };

        std::function<void(Viewport*)> in_viewport_callback_{};
    };

    class SceneViewport : public Viewport
    {

    };
}
