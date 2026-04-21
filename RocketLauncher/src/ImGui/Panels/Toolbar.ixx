module;
export module Toolbar;

import rke;

export namespace rke
{
    class IconButton
    {
    public:
        friend class Toolbar;

        IconButton(String name, String id, Ref<Texture2D> icon,
                   std::function<void(IconButton*)> on_click,
                   std::function<bool()> is_enabled, bool visible);

        void set_icon(Ref<Texture2D> icon) { icon_ = icon; }
    private:
        void render(float size); // only square supported now
    private:
        String name_{};
        String id_	 {};

        Ref<Texture2D> icon_{};

        std::function<void(IconButton*)> on_click_{};
        std::function<bool()> is_enabled_{};
        bool visible_{ true };
    };

    class Toolbar : public Panel
    {
    public:
        Toolbar(String name) : Panel(std::move(name)) {}

        void on_imgui_render() override;
        void emplace_icon_button(String name,
            Ref<Texture2D> icon,
            std::function<void(IconButton*)> on_click,
            std::function<bool()> is_enabled = nullptr,
            bool visible = true);
    private:
        std::vector<IconButton> icon_buttons_{};
    };
}
