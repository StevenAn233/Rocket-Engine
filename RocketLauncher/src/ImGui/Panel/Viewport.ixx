module;
export module Viewport;

import rke;

export namespace rke
{
    class Viewport : public Panel
    {
    public:
        using ViewportCallback = std::function<void(Viewport*)>;
        using TargetGetter = std::function<const Texture2D*()>;

        Viewport(String name) : Panel(std::move(name)) {}

        void on_imgui_render() override;

        inline void set_viewport_callback(ViewportCallback callback)
            { callback_ = std::move(callback); }
        inline void set_target_getter(TargetGetter getter)
            { getter_ = std::move(getter); }
    private:
        ViewportCallback callback_{};
        TargetGetter getter_{};
    };
}
