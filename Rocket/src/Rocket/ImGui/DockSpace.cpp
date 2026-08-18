module;
module DockSpace;

import Log;
import Input;
import FileUtils;
import ConfigProxy;
import Application;
import Project;
import ImGuiSetup;
import ApplicationEvent;
import ProjectEvent;

namespace rke
{
    DockSpace::DockSpace(String name, Path config_path, NativeWindow context)
        : name_(std::move(name))
        , config_path_(std::move(config_path))
    {
        imgui::init(context);

        if(!config_path_.exists())
            { CORE_WARN(u8"DockSpace: File \'{}\' not found!", config_path_); return; }
        auto reader{ ConfigReader::create(config_path_) };
        if(!reader || !reader->is_map())
            { CORE_WARN(u8"DockSpace: File format incorrect!"); return; }
        flags_ = reader->get_at(u8"DockSpace Flags", 0);
        panel_registry_.deserialize_from(*(reader.get()));

        project_creating_modal_.set_project_created_callback
        ([this](const Path& rkproj_path) { app().load_project(rkproj_path); });

        modal_registry_.register_modal(&project_creating_modal_,
        {[this]() -> bool {
            if(to_create_project_) {
                to_create_project_ = false;
                return true;
            }
            return false;
        }});
    }

    DockSpace::~DockSpace()
    {
        modal_registry_.unregister_modal(&project_creating_modal_);

        if(config_path_.empty()) return;
        file::check_to_create_dir(config_path_);

        auto writer{ ConfigWriter::create() };
        if(!writer) { CORE_ERROR(u8"Dockspace: "
            u8"Failed to create config writer!"); return; }
        writer->begin_map();
        writer->write(u8"DockSpace Flags", static_cast<int>(flags_));
        panel_registry_.serialize_to(*(writer.get()));
        writer->push_to_file(config_path_);

        imgui::shutdown();
    }

    void DockSpace::render(glm::vec2 offset, glm::vec2 scale)
    {
        static bool enable_dockspace{ true };

        imgui::begin_render();

        const ImGuiViewport* viewport{ ImGui::GetMainViewport() };
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
        if(flags_ & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // style setting(no rounding and bordersize and padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding  , 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin(name_.raw(), &enable_dockspace, window_flags);

        ImGuiIO& io{ ImGui::GetIO() };
        if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id{ ImGui::GetID(name_.raw()) };
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), flags_, nullptr);
        }
        ImGui::PopStyleVar(3);

        if(ImGui::BeginMenuBar())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing  , ImVec2(12.0f, 4.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2( 8.0f, 4.0f)); // for menus

            if(ImGui::BeginMenu("Window"))
            {
                if(ImGui::MenuItem("Close", "Alt+F4"))
                {
                    WindowClosedEvent e{ u8"main" };
                    app().send_event(e);
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Docking"))
            {
            // MenuItem: const char* label, const char* shortcut, bool selected, bool enabled
                if(ImGui::MenuItem("No docking in the center", "",
                    (flags_ & ImGuiDockNodeFlags_NoDockingOverCentralNode) != 0))
                    { flags_ ^= ImGuiDockNodeFlags_NoDockingOverCentralNode; }
                if(ImGui::MenuItem("No docking split", "",
                    (flags_ & ImGuiDockNodeFlags_NoDockingSplit) != 0))
                    { flags_ ^= ImGuiDockNodeFlags_NoDockingSplit; }
                if(ImGui::MenuItem("No undocking", "",
                    (flags_ & ImGuiDockNodeFlags_NoUndocking) != 0))
                    { flags_ ^= ImGuiDockNodeFlags_NoUndocking; }
                if(ImGui::MenuItem("No resize", "",
                    (flags_ & ImGuiDockNodeFlags_NoResize) != 0))
                    { flags_ ^= ImGuiDockNodeFlags_NoResize; }
                if(ImGui::MenuItem("Hide tab bar auto", "",
                    (flags_ & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
                    { flags_ ^= ImGuiDockNodeFlags_AutoHideTabBar; }
                if(ImGui::MenuItem("Pass throuth central node", "",
                    (flags_ & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
                    { flags_ ^= ImGuiDockNodeFlags_PassthruCentralNode; }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Panels"))
            {
                panel_registry_.render_switches_menubar();
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Project"))
            {
                bool running{ editor_runtime_ ? editor_runtime_() : false };
                if(ImGui::MenuItem("New Project..." , "Ctrl+N", false, !running)) create_project();
                if(ImGui::MenuItem("Open Project...", "Ctrl+O", false, !running))
                    open_project(app().get_windows_lib().get_main());
                
                if(ImGui::MenuItem("Save Project", "Ctrl+S",
                    false, !running && app().get_project())) save_project();
                ImGui::EndMenu();
            }

            ImGui::PopStyleVar(2);
            ImGui::EndMenuBar();
        }
        ImGui::End();

        panel_registry_.render_all();
        modal_registry_.render_all();

        imgui::end_render();
    }

    bool DockSpace::should_block_mouse() const
    {
        for(const auto& [handle, attrib] : panel_registry_.attribs_)
        {
            if(attrib.dont_block_when_hovered &&
               handle->is_hovered()) return false;
        }
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool DockSpace::should_block_keyboard() const
    {
        if(ImGui::GetIO().WantTextInput) return true;
        if(project_creating_modal_.in_use()) return true;
        for(const auto& [handle, attrib] : panel_registry_.attribs_)
        {
            if(attrib.dont_block_when_focused &&
               handle->is_focused()) return false;
        }
        return ImGui::GetIO().WantCaptureKeyboard;
    }

    void DockSpace::on_update()
    {
        ctrl_pressed_ = (
            Input::is_key_pressed(Key::RightControl) ||
            Input::is_key_pressed(Key::LeftControl )
        );
    }

    bool DockSpace::on_key_pressed(KeyPressedEvent& e)
    {
        if(e.is_held()) return false;
        if(editor_runtime_ && editor_runtime_()) return false;
        switch(e.get_key())
        {
        case Key::N:
            if(ctrl_pressed_) { create_project(); return true; }
            return false;
        case Key::O:
            if(ctrl_pressed_) {
                open_project(app().get_windows_lib().get_main());
                return true;
            }
            return false;
        case Key::S:
            if(ctrl_pressed_) { save_project(); return true; }
            return false;
        }
        return false;
    }

    void DockSpace::create_project() { to_create_project_ = true; }

    void DockSpace::open_project(const Window& window)
    {
        String str{ file::dialogs::open_file (
            u8"Rocket Project (*.rkproj)|*.rkproj|",
            window.get_context()
        )};
        if(!str.empty()) app().load_project(Path(str));
    }

    void DockSpace::save_project()
    {
        Project* project{ app().get_project() };
        if(project) {
            project->save();
            ProjectSavedEvent e{ u8"main" };
            app().send_event(e); 
        }
    }
}
