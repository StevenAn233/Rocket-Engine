module;
export module ContentBrowserPanel;

import rke;

export namespace rke
{
    class ContentBrowserPanel : public Panel
    {
    public:
        ContentBrowserPanel(String name);
        ~ContentBrowserPanel();

        bool set_context(const Path& context);
        void on_imgui_render() override;

        void set_folder_icon(const Path& filepath);
        void set_image_icon (const Path& filepath);
        void set_file_icon  (const Path& filepath);

        void load_from(Path filepath);
    private:
        Ref<Texture2D> get_file_icon(const String& file_name);
        void scale_icon(float extent = 1.0f)
            { thumbnail_scale_ *= std::sqrt(extent); }
    private:
        Path filepath_{}; // for save

        Path context_{}; // project assets path
        Path current_path_{};

        Ref<Texture2D> folder_icon_{};
        Ref<Texture2D> image_icon_ {};
        Ref<Texture2D> file_icon_  {};
        static constexpr float basic_thumbnail_size_{ 96.0f };
        float thumbnail_scale_{ 1.0f };
    };
}
