module;
module DockSpace;

namespace rke
{
    DockSpace::~DockSpace()
    {
        if(filepath_.empty()) return;
        file::check_to_create_dir(filepath_);

        auto writer{ ConfigWriter::create() };
        if(!writer) {
            CORE_ERROR(u8"Dockspace: Failed to create config writer!");
            return;
        }
        writer->begin_map();
        writer->write(u8"DockSpace Flags", static_cast<int>(dockspace_flags_));
        writer->end_map();

        writer->push_to_file(filepath_);
    }

    void DockSpace::load_from(Path path)
    {
        filepath_ = std::move(path);
        if(!filepath_.exists()) {
            CORE_WARN(u8"DockSpace: File \'{}\' not found!", filepath_);
            return;
        }
        auto reader{ ConfigReader::create(filepath_) };
        if(!reader || !reader->is_map()) {
            CORE_WARN(u8"DockSpace: File format incorrect!");
            return;
        }
        dockspace_flags_ = ImGuiDockNodeFlags(reader->get_at(u8"DockSpace Flags", 0));
    }

    void DockSpace::on_imgui_render(glm::vec2 offset, glm::vec2 scale)
    {
        static bool enable_dockspace{ true };

        const ImGuiViewport* viewport{ ImGui::GetMainViewport() }; // glfwWindow(attached in ImGuiLayer)
        ImGui::SetNextWindowPos // not including menu bar/task bar
        ({
            viewport->WorkPos.x + offset.x,
            viewport->WorkPos.y + offset.y
        });
        ImGui::SetNextWindowSize // not including menu bar/tast bar
        ({
            viewport->WorkSize.x * scale.x,
            viewport->WorkSize.y * scale.y
        });
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags
        {	ImGuiWindowFlags_NoDocking
          | ImGuiWindowFlags_NoTitleBar
          | ImGuiWindowFlags_MenuBar
          | ImGuiWindowFlags_NoCollapse
          | ImGuiWindowFlags_NoResize
          | ImGuiWindowFlags_NoMove
          | ImGuiWindowFlags_NoBringToFrontOnFocus
          | ImGuiWindowFlags_NoNavFocus
        };
        if(dockspace_flags_ & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // style setting(no rounding and bordersize and padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding  , 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin(name_.raw(), &enable_dockspace, window_flags);

        ImGuiIO& io{ ImGui::GetIO() };
        if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id{ ImGui::GetID("MyDockSpace") };
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags_, nullptr);
        }
        ImGui::PopStyleVar(3);

        if(ImGui::BeginMenuBar())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing  , ImVec2(12.0f, 4.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2( 8.0f, 4.0f)); // for menus

            if(menubar_callback_) menubar_callback_();
            if(ImGui::BeginMenu("Docking"))
            {
                // MenuItem: const char* label, const char* shortcut, bool selected, bool enabled
                if(ImGui::MenuItem("No docking in the center", "",
                (dockspace_flags_ & ImGuiDockNodeFlags_NoDockingOverCentralNode) != 0))
                    { dockspace_flags_ ^= ImGuiDockNodeFlags_NoDockingOverCentralNode; }
                if(ImGui::MenuItem("No docking split", "",
                (dockspace_flags_ & ImGuiDockNodeFlags_NoDockingSplit) != 0))
                    { dockspace_flags_ ^= ImGuiDockNodeFlags_NoDockingSplit; }
                if(ImGui::MenuItem("No undocking", "",
                (dockspace_flags_ & ImGuiDockNodeFlags_NoUndocking) != 0))
                    { dockspace_flags_ ^= ImGuiDockNodeFlags_NoUndocking; }
                if(ImGui::MenuItem("No resize", "",
                (dockspace_flags_ & ImGuiDockNodeFlags_NoResize) != 0))
                    { dockspace_flags_ ^= ImGuiDockNodeFlags_NoResize; }
                if(ImGui::MenuItem("Hide tab bar auto", "",
                (dockspace_flags_ & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
                    { dockspace_flags_ ^= ImGuiDockNodeFlags_AutoHideTabBar; }
                if(ImGui::MenuItem("Pass throuth central node", "",
                (dockspace_flags_ & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
                    { dockspace_flags_ ^= ImGuiDockNodeFlags_PassthruCentralNode; }
                ImGui::EndMenu();
            }
            ImGui::PopStyleVar(2);

            ImGui::EndMenuBar();
        }

        ImGui::End();
    }
}
