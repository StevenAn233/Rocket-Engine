module;
export module Toolbar;

import rke;

export namespace rke
{
    class IconButton
    {
    public:
        friend class Toolbar;

        IconButton(String name, String str_id, Scope<Texture> icon,
                   std::function<void(IconButton*)> on_click,
                   std::function<bool()> is_enabled, bool visible);
    private:
        void render(float size); // only square supported now
    private:
        String name_;
        String str_id_;

        Scope<Texture> icon_;
        GTexture* gtex_cache_{}; // may modify
        std::function<void(IconButton*)> on_click_;
        std::function<bool()> is_enabled_;
        bool visible_;
    };

    class Toolbar : public Panel
    {
    public:
        Toolbar(String name) : Panel(std::move(name)) {}

        void emplace_icon_button(String name,
            const Path& icon_path,
            std::function<void(IconButton*)> on_click,
            std::function<bool()> is_enabled = nullptr,
            bool visible = true);
    private:
        void on_imgui_render() override;
    private:
        std::vector<IconButton> icon_buttons_{};
    };
}
