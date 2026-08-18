module;
module ContentBrowserPanel;

namespace {
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

        constexpr float padding{ 16.0f };
        constexpr float basic_thumbnail_size_{ 96.0f };
        float thumbnail_size{ basic_thumbnail_size_ * thumbnail_scale_ };

        float panel_w{ ImGui::GetContentRegionAvail().x };
        int colunm_count{ static_cast<int>(panel_w / (padding + thumbnail_size))};
        if(colunm_count < 1) colunm_count = 1;

        ImGui::BeginChild("Content", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);

        bool to_create_scene{ false };
        if((current_dir_ == (assets_dir_ / u8"scenes")) &&
           ImGui::BeginPopupContextWindow("ContentBrowserPopup",
           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if(ImGui::MenuItem("New Scene")) to_create_scene = true;
            ImGui::EndPopup();
        }
        if(to_create_scene) ImGui::OpenPopup("New Scene Name");
        if(ImGui::BeginPopupModal("New Scene Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter Scene Name:");
            ImGui::InputText("##SceneName", name_buffer_->data(), name_buffer_->size(),
                ImGuiInputTextFlags_EnterReturnsTrue);
            if(ImGui::Button("Create"))
            {
                String scene_name{ str::to_char8(name_buffer_->data()) };
                if(!scene_name.empty())
                {
                    Path new_scene_path{ current_dir_ / (scene_name + u8".rkscene")};
                    if(new_scene_path.exists())
                        CORE_WARN(u8"ContentBrowserPanel: "
                            u8"Scene '{}' already exists! "
                            u8"Please choose an another name.",
                        new_scene_path);
                    else {
                        Scope<Scene> new_scene{ context_->load_scene(scene_name, initializer_) };
                        initializer_.serialize(*new_scene, new_scene_path);
                        new_scene.reset();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::SameLine();
            if(ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::Columns(colunm_count, 0, false);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        for(auto& entry : fs::directory_iterator(current_dir_))
        {
            Path path{ entry.path() };
            if(path.extension() == u8".meta") continue;

            String file_name{ Path(entry.path().filename()).string() };
            uint32 icon_id{ entry.is_directory() ?
                folder_icon_->get_renderer_id () :
                get_file_icon(file_name)->get_renderer_id() };
            ImGui::ImageButton(file_name.raw(),
                static_cast<ImTextureID>(icon_id),
                { thumbnail_size, thumbnail_size }, { 0, 1 }, { 1, 0 },
                { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }
            );
        
            if(path.extension() == u8".rkscene")
            {
                if(ImGui::BeginPopupContextItem())
                {
                    if(ImGui::MenuItem("Delete Scene"))
                    {
                        fs::remove(path);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            
            ImGui::PushID(path.string().raw());
            if(!entry.is_directory() && ImGui::BeginDragDropSource())
            {
                if(path.extension() == u8".rkscene") {
                    String scene_name{ path.stem().string() };
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_SCENE",
                        scene_name.raw(), scene_name.size() + 1, ImGuiCond_Once);
                } else {
                    AssetUUID asset_uuid{ context_ ?
                        context_->get_assets_manager().get_asset_uuid(path) : AssetUUID(0) };
                    if(!asset_uuid.empty()) {
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET",
                            &asset_uuid, sizeof(AssetUUID), ImGuiCond_Once);
                    }
                }
                
                ImGui::GetWindowDrawList()->AddCallback
                    (ImGui_ImplOpenGL3_DisableBindSampler, nullptr);
                ImGui::Image (
                    std::bit_cast<void*>(static_cast<uint64>(icon_id)),
                    { thumbnail_size, thumbnail_size },
                    { 0, 1 }, { 1, 0 }
                );
                ImGui::GetWindowDrawList()->AddCallback
                    (ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);

                ImGui::Text("%s", file_name.raw());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();

            if(entry.is_directory() && ImGui::IsItemHovered()
            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                { current_dir_ /= path.filename(); }
            ImGui::TextWrapped(file_name.raw());
            ImGui::NextColumn();
        }
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::End();
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
}
