module;
export module Viewport;

import rke;

export namespace rke
{
    class Viewport : public Panel
    {
    public:
        using ViewportCallback = std::function<void(Viewport&)>;
        using TargetGetter = std::function<const Texture2D*()>;

        Viewport(String name, TargetGetter getter);

        inline void set_viewport_callback(ViewportCallback callback)
            { callback_ = std::move(callback); }
    private:
        void on_imgui_render() override;
    private:
        TargetGetter getter_;
        ViewportCallback callback_{};
    };
}
