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
    private:
        uint32 render_target_id_{};
        std::function<void(Viewport*)> in_viewport_callback_{};
    };

//  class SceneViewport : public Viewport
//  {
//
//  };
}
