module;
module ContentBrowserPanel;

namespace {
    constexpr float padding{ 16.0f };
    constexpr float basic_thumbnail_size{ 96.0f };

    static void ImGui_ImplOpenGL3_DisableBindSampler(const ImDrawList*, const ImDrawCmd*)
        { rke::imgui::disable_bind_sampler(); }
}

namespace rke
{
    ContentBrowserPanel::ContentBrowserPanel(String name)
        : Panel(std::move(name))
    {
        name_buffer_ = create_scope<std::array<char, 256>>();
        std::memcpy(name_buffer_->data(), "Untitled", 9);
    }

    ContentBrowserPanel::~ContentBrowserPanel()
    {
        if(filepath_.empty()) return;
        file::check_to_create_dir(filepath_);

        auto writer{ ConfigWriter::create() };
        if(!writer) {
            CORE_ERROR(u8"ContentBrowserPanel: Failed to create config writer!");
            return;
        }
        writer->begin_map();
        writer->write(u8"Thumbnail Scale", thumbnail_scale_);
        writer->end_map();

        writer->push_to_file(filepath_);
    }

    void ContentBrowserPanel::on_project_loaded()
    {
        context_ = app().get_project();
        if(!context_) {
            CORE_ERROR(u8"ContentBrowerPanel: Project null!");
            assets_dir_.clear();
            current_dir_.clear();
            return;
        }
        assets_dir_ = context_->get_assets_dir();
        if(!assets_dir_.exists()) {
            CORE_ERROR(u8"ContentBrowserPanel: "
                u8"Assets dir '{}' doesn't exsit!", assets_dir_);
            assets_dir_.clear();
        }
        current_dir_ = assets_dir_;
    }

    void ContentBrowserPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());
        if(assets_dir_.empty()) { ImGui::End(); return; }

        ImGui::BeginChild("Header",
            ImVec2(0, ImGui::CalcTextSize("hello world").y),
            ImGuiChildFlags_None, ImGuiWindowFlags_None);

        if(ImGui::SmallButton("-")) scale_icon(1.0f / 1.25f);
        ImGui::SameLine();
        if(ImGui::SmallButton("+")) scale_icon(1.25f);
        ImGui::SameLine();

        if(current_dir_ != assets_dir_) {
            if(ImGui::SmallButton("<"))
                current_dir_ = current_dir_.parent_path();
        } else {
            ImGui::BeginDisabled();
            ImGui::SmallButton("<");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        ImGui::Text("%s", current_dir_.string().raw());

        ImGui::EndChild();

        float thumbnail_size{ basic_thumbnail_size * thumbnail_scale_ };
        float panel_w{ ImGui::GetContentRegionAvail().x };
        int colunm_count{ static_cast<int>(panel_w / (padding + thumbnail_size))};
        if(colunm_count < 1) colunm_count = 1;

        ImGui::BeginChild("Content", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);

        bool to_create_new_scene{ false };
        if(ImGui::BeginPopupContextWindow("ContentBrowserPopup",
           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if(ImGui::MenuItem("New Scene"))
            {
                to_create_new_scene = true;
                current_dir_ = context_->get_scenes_dir();
            }
            ImGui::Separator();
            if(ImGui::MenuItem("Refresh"))
                context_->get_assets_manager().rescan(assets_dir_); // TO MODIFY
            ImGui::EndPopup();
        }
        if(to_create_new_scene) ImGui::OpenPopup("New Scene Name");
        new_scene_modal();

        ImGui::Columns(colunm_count, 0, false);

        ImGui::GetWindowDrawList()->AddCallback
            (ImGui_ImplOpenGL3_DisableBindSampler, nullptr);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        for(auto& entry : fs::directory_iterator(current_dir_))
        {
            Path path{ entry.path() };
            String filename{};
            uint32 icon_handle{};
            void (*image_button_attachment)(ContentBrowserPanel&, const String&, const Path&, uint32){};
            if(entry.is_directory())
            {
                filename = Path(path.filename()).string();
                icon_handle = folder_icon_->get_renderer_id();
                image_button_attachment = &ContentBrowserPanel::entry_is_directory;
            }
            else if(path.extension() == u8".rkscene")
            {
                filename = Path(path.filename()).string();
                icon_handle = get_file_icon(filename)->get_renderer_id();
                image_button_attachment = &ContentBrowserPanel::entry_is_rkscene;
            }
            else if(path.extension() == u8".meta")
            {
                filename = Path(path.stem()).string(); // with suffix
                icon_handle = get_file_icon(filename)->get_renderer_id();
                image_button_attachment = &ContentBrowserPanel::entry_is_meta;
            }
            else continue;
            
            ImGui::ImageButton(filename.raw(),
                static_cast<ImTextureID>(icon_handle),
                { thumbnail_size, thumbnail_size }, { 0, 1 }, { 1, 0 },
                { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }
            );
            if(image_button_attachment) image_button_attachment(*this, filename, path, icon_handle);

            ImGui::TextWrapped(filename.raw());
            ImGui::NextColumn();            
        }
        ImGui::PopStyleColor();
        ImGui::GetWindowDrawList()->AddCallback
            (ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);
        
        ImGui::EndChild();
        ImGui::End();
    }

    void ContentBrowserPanel::new_scene_modal()
    {
        if(ImGui::BeginPopupModal("New Scene Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter Scene Name:");
            ImGui::InputText("##SceneName", name_buffer_->data(), name_buffer_->size());
            if(ImGui::Button("Create")) {
                context_->create_scene(String(str::to_char8(name_buffer_->data())));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if(ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    // srgb has to be false!!!(and only srgb textures are supported here)
    void ContentBrowserPanel::set_folder_icon(const Path& filepath)
    {
        folder_icon_ = Texture2D::create(filepath,
            Texture::FiltFormat::Linear,
            Texture::WrapFormat::Clamp2Edge, false);
    }

    void ContentBrowserPanel::set_image_icon(const Path& filepath)
    {
        image_icon_ = Texture2D::create(filepath,
            Texture::FiltFormat::Linear,
            Texture::WrapFormat::Clamp2Edge, false);
    }

    void ContentBrowserPanel::set_file_icon(const Path& filepath)
    {
        file_icon_ = Texture2D::create(filepath,
            Texture::FiltFormat::Linear,
            Texture::WrapFormat::Clamp2Edge, false);
    }

    void ContentBrowserPanel::load_from(Path filepath)
    {
        filepath_ = std::move(filepath);
        if(!filepath_.exists()) {
            CORE_WARN(u8"ContentBrowserPanel: File '{}' not found!", filepath_);
            return;
        }
        auto reader{ ConfigReader::create(filepath_) };
        if(!reader || !reader->is_map()) {
            CORE_WARN(u8"ContentBrowerPanel: File format incorrect!");
            return;
        }
        thumbnail_scale_ = reader->get_at(u8"Thumbnail Scale", thumbnail_scale_);
    }

    Texture2D* ContentBrowserPanel::get_file_icon(const String& file_name)
    {
        if(file_name.ends_with(u8".png") || file_name.ends_with(u8".jpg"))
            return image_icon_.get();
        return file_icon_.get();
    }

    void ContentBrowserPanel::entry_is_directory(this ContentBrowserPanel& self,
        const String& filename, const Path& path, uint32 icon_handle)
    {
        if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            self.current_dir_ /= filename;
    }

    void ContentBrowserPanel::entry_is_rkscene(this ContentBrowserPanel& self,
        const String& filename, const Path& path, uint32 icon_handle)
    {
        if(ImGui::BeginPopupContextItem())
        {
            if(ImGui::MenuItem("Delete Scene"))
            {
                fs::remove(path);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
            return;
        }

        ImGui::PushID(filename.raw());
        if(ImGui::BeginDragDropSource())
        {
            String scene_name{ path.stem().string() };
            ImGui::SetDragDropPayload("CONTENT_BROWSER_SCENE",
                scene_name.raw(), scene_name.size() + 1, ImGuiCond_Once);

            float thumbnail_size{ basic_thumbnail_size * self.thumbnail_scale_ };
            ImGui::Image(std::bit_cast<void*>(static_cast<uint64>(icon_handle)),
                { thumbnail_size, thumbnail_size }, { 0, 1 }, { 1, 0 });
            ImGui::Text("%s", filename.raw());

            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }

    void ContentBrowserPanel::entry_is_meta(this ContentBrowserPanel& self,
        const String& filename, const Path& path, uint32 icon_handle)
    {
        Path asset_path{ path.parent_path() / filename };
        ImGui::PushID(filename.raw());
        if(ImGui::BeginDragDropSource())
        {
            AssetUUID asset_uuid{ self.context_ ?
                self.context_->get_assets_manager()
                    .get_asset_uuid(asset_path) : AssetUUID(0)
            };
            if(!asset_uuid.empty()) {
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET",
                    &asset_uuid, sizeof(AssetUUID), ImGuiCond_Once);
            }
            
            float thumbnail_size{ basic_thumbnail_size * self.thumbnail_scale_ };
            ImGui::Image(std::bit_cast<void*>(static_cast<uint64>(icon_handle)),
                { thumbnail_size, thumbnail_size }, { 0, 1 }, { 1, 0 });
            ImGui::Text("%s", filename.raw());

            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }
}
