module;
export module ContentBrowserPanel;

import rke;

export namespace rke
{
    class ContentBrowserPanel : public Panel
    {
    public:
        ContentBrowserPanel(String name, std::function<const Path&()> func);
        ~ContentBrowserPanel();

        void set_folder_icon(const Path& filepath);
        void set_image_icon (const Path& filepath);
        void set_file_icon  (const Path& filepath);

        void on_project_loaded();
        void load_from(Path filepath);
    private:
        void on_imgui_render() override;
        Texture2D* get_file_icon(const String& file_name);

        inline void scale_icon(float extent = 1.0f)
            { thumbnail_scale_ *= std::sqrt(extent); }
    private:
        Path filepath_{};

        Path context_{}; // project assets dir
        Path current_path_{};

        Scope<Texture2D> folder_icon_{};
        Scope<Texture2D> image_icon_ {};
        Scope<Texture2D> file_icon_  {};
        float thumbnail_scale_{ 1.0f };

        Scope<std::array<char, 256>> name_buffer_{};
        SceneSerializer initializer_{};

        std::function<const Path&()> current_scene_path_getter_;
    };
}
