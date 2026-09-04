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

        void set_folder_icon(const Path& filepath);
        void set_image_icon (const Path& filepath);
        void set_file_icon  (const Path& filepath);

        void on_project_loaded();
        void load_from(Path filepath);
    private:
        void on_imgui_render() override;

        void new_scene_modal();
        GTexture* get_file_icon_gtex(const String& file_name);

        inline void scale_icon(float extent = 1.0f)
            { thumbnail_scale_ *= std::sqrt(extent); }

        void entry_is_directory(this ContentBrowserPanel& self,
            const String& filename, const Path& path, uint32 icon_handle);
        void entry_is_rkscene(this ContentBrowserPanel& self,
            const String& filename, const Path& path, uint32 icon_handle);
        void entry_is_meta(this ContentBrowserPanel& self,
            const String& filename, const Path& path, uint32 icon_handle);
    private:
        Path filepath_{};

        Project* context_{};
        Path assets_dir_{}; // project assets dir
        Path current_dir_{};

        float thumbnail_scale_{ 1.0f };

        Scope<Texture> folder_icon_{};
        Scope<Texture> image_icon_{};
        Scope<Texture> file_icon_{};
    // may modify
        GTexture* folder_gtex_cache_{};
        GTexture* image_gtex_cache_{};
        GTexture* file_gtex_cache_{};

        Scope<GTexture2D> default_icon_gtex_{};
        
        Scope<std::array<char, 256>> name_buffer_{};
    };
}
