module;
module ContentBrowserPanel;

namespace rke
{
    ContentBrowserPanel::ContentBrowserPanel(String name)
        : Panel(std::move(name)) {}

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

    bool ContentBrowserPanel::set_context(const Path& context)
    {
        if(!context.exists()) {
            CORE_ERROR(u8"ContentBrowserPanel: File '{}' doesn't exsit!", context);
            context_.clear();
            current_path_.clear();
            return false;
        }
        context_ = context;
        current_path_ = context;
        return true;
    }

    void ContentBrowserPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());
        if(context_.empty()) { ImGui::End(); return; }

        ImGui::BeginChild("Header",
            ImVec2(0, ImGui::CalcTextSize("hello world").y),
            ImGuiChildFlags_None, ImGuiWindowFlags_None);

        if(ImGui::SmallButton("-")) scale_icon(1.0f / 1.25f);
        ImGui::SameLine();
        if(ImGui::SmallButton("+")) scale_icon(1.25f);
        ImGui::SameLine();

        if(current_path_ != context_) {
            if(ImGui::SmallButton("<"))
                current_path_ = current_path_.parent_path();
        } else {
            ImGui::BeginDisabled();
            ImGui::SmallButton("<");
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        ImGui::Text("%s", current_path_.string().raw());

        ImGui::EndChild();

        constexpr float PADDING{ 16.0f };
        float thumbnail_size{ basic_thumbnail_size_ * thumbnail_scale_ };

        float panel_w{ ImGui::GetContentRegionAvail().x };
        int colunm_count{ static_cast<int>(panel_w / (PADDING + thumbnail_size))};
        if(colunm_count < 1) colunm_count = 1;

        ImGui::BeginChild("Content", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);

        bool to_create_scene{ false };
        if((current_path_ == (context_ / u8"scenes")) &&
           ImGui::BeginPopupContextWindow("ContentBrowserPopup",
           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if(ImGui::MenuItem("New Scene")) to_create_scene = true;
            ImGui::EndPopup();
        }
        if(to_create_scene) ImGui::OpenPopup("New Scene Name");
        if(ImGui::BeginPopupModal("New Scene Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char name_buffer[256]{ "Untitled" };
            ImGui::Text("Enter Scene Name:");
            ImGui::InputText("##SceneName", name_buffer, sizeof(name_buffer),
                ImGuiInputTextFlags_EnterReturnsTrue);
            if(ImGui::Button("Create")) {
                String scene_name{ str::to_char8(name_buffer) };
                if(!scene_name.empty()) {
                    Ref<Scene> new_scene{ create_ref<Scene>(scene_name) };
                    SceneSerializer serializer(new_scene);
                    Path new_scene_path{ current_path_ / (scene_name + u8".rkscene")};
                    if(!new_scene_path.exists()) {
                        serializer.serialize(new_scene_path);

                        ImGui::CloseCurrentPopup();
                        strncpy(name_buffer, "Untitled", sizeof(name_buffer) - 1); // recover
                        name_buffer[sizeof(name_buffer) - 1] = '\0';
                    }
                    else CORE_WARN(u8"ContentBrowserPanel: Scene '{}' already exists! "
                        u8"Please choose an another name.", new_scene_path);
                }
            }
            ImGui::SameLine();
            if(ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                strncpy(name_buffer, "Untitled", sizeof(name_buffer) - 1); // recover
                name_buffer[sizeof(name_buffer) - 1] = '\0';
            }
            ImGui::EndPopup();
        }

        ImGui::Columns(colunm_count, 0, false);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        for(auto& entry : fs::directory_iterator(current_path_))
        {
            if(entry.path().extension() == u8".meta") continue;

            String file_name{ Path(entry.path().filename()).string() };
            uint32 icon_id{ entry.is_directory() ?
                folder_icon_->get_renderer_id () :
                get_file_icon(file_name)->get_renderer_id() };
            ImGui::ImageButton(file_name.raw(), static_cast<ImTextureID>(icon_id),
                              { thumbnail_size, thumbnail_size }, { 0, 1 }, { 1, 0 },
                              { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f });

            Project* active_project{ Project::get_active_project() };
            if(entry.path().extension() == u8".rkscene" && active_project) {
                Path active_scene_path{ active_project->get_active_scene_path() };
                if(!fs::equivalent(entry.path(), active_scene_path.get()))
                {
                    if(ImGui::BeginPopupContextItem())
                    {
                        if(ImGui::MenuItem("Delete Scene"))
                        {
                            fs::remove(entry.path());
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
            }

            Path filepath{ entry.path() };
            String filepath_str{ filepath.string() };
            ImGui::PushID(filepath_str.raw());
            if(!entry.is_directory() && ImGui::BeginDragDropSource())
            {
                if(filepath.extension() == u8".rkscene") {
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_SCENE",
                        filepath_str.raw(), filepath_str.size() + 1, ImGuiCond_Once);
                } else {
                    AssetUUID asset_uuid{ AssetsManager::get_asset_uuid(filepath) };
                    if(!asset_uuid.empty()) {
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET",
                            &asset_uuid, sizeof(AssetUUID), ImGuiCond_Once);
                    }
                }
                
                ImGui::Image(std::bit_cast<void*>(static_cast<uint64>(icon_id)),
                    { thumbnail_size, thumbnail_size }, { 0, 1 }, { 1, 0 });
                ImGui::Text("%s", file_name.raw());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();

            if(entry.is_directory() && ImGui::IsItemHovered()
            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                { current_path_ /= entry.path().filename(); }
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

    Ref<Texture2D> ContentBrowserPanel::get_file_icon(const String& file_name)
    {
        if(file_name.ends_with(u8".png") || file_name.ends_with(u8".jpg"))
            return image_icon_;
        return file_icon_;
    }
}
