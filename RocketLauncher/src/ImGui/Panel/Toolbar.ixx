module;
export module Toolbar;

import rke;

export namespace rke
{
    class IconButton
    {
    public:
        friend class Toolbar;

        IconButton(String name, String str_id, Scope<Texture2D> icon,
                   std::function<void(IconButton*)> on_click,
                   std::function<bool()> is_enabled, bool visible);
    private:
        void render(float size); // only square supported now
    private:
        String name_;
        String str_id_;

        Scope<Texture2D> icon_;
        std::function<void(IconButton*)> on_click_;
        std::function<bool()> is_enabled_;
        bool visible_;
    };

    class Toolbar : public Panel
    {
    public:
        Toolbar(String name) : Panel(std::move(name)) {}

        void emplace_icon_button(String name,
            Scope<Texture2D> icon,
            std::function<void(IconButton*)> on_click,
            std::function<bool()> is_enabled = nullptr,
            bool visible = true);
    private:
        void on_imgui_render() override;
    private:
        std::vector<IconButton> icon_buttons_{};
    };
}
