import rke;
import DockSpaceLayer;
import EditorLayer;

import LogPanel;
import ApplicationPanel;
import ProjectSettingPanel;

namespace rke
{
    class RocketLauncher : public Application
    {
    public:
        RocketLauncher() : Application() {}
        ~RocketLauncher() override {}

        void init() override;
        void shutdown() override;
        void on_main_window_closing() override;
    private:
        EditorLayer* editor_layer_{};

        Scope<LogPanel> log_panel_{};
        Scope<ApplicationPanel> app_panel_{};
        Scope<ProjectSettingPanel> proj_panel_{};
    };

    void RocketLauncher::init()
    {
        Window& main_window{ get_windows_lib().load_main
        (
            create_scope<Window::Props>(Window::Props
            {
                .title{ u8"Rocket Engine" },
                .icon_path{ file::assets_dir() / u8"icons" / u8"RKE.png" },
                .width{ 2450 }, .height{ 1300 },
                .x_coord{ 50 }, .y_coord{ 50 }
            })
        )};

        Scope<DockSpaceLayer> ds_layer{ create_scope<DockSpaceLayer>
        (
            u8"Dockspace Layer", &main_window,
            file::editor_dir() / u8"settings" / u8"dockspace.yaml"
        )};
        dockspace_ = &(ds_layer->dockspace_);
        main_window.push_overlay(Scope<Layer>(ds_layer.release()));

        Scope<EditorLayer> editor_layer{ create_scope
            <EditorLayer>(u8"EditorLayer", &main_window) };
        editor_layer_ = editor_layer.get();
        main_window.push_layer(Scope<Layer>(editor_layer.release()));
        dockspace_->set_editor_runtime_func
            ([this]() { return editor_layer_->testing(); });

        log_panel_  = create_scope<LogPanel>(u8"Log");
        app_panel_  = create_scope<ApplicationPanel>(u8"Application");
        proj_panel_ = create_scope<ProjectSettingPanel>(u8"Project Settings");
        log_panel_->load_from(file::editor_dir() / u8"settings" / u8"log.yaml");

        register_panel(log_panel_ .get());
        register_panel(app_panel_ .get());
        register_panel(proj_panel_.get());
        register_panel(main_window.get_panel_handle());

        RKE_INFO(u8"Welcome to Rocket Engine!");
    }

    void RocketLauncher::shutdown() { RKE_INFO(u8"Bye!"); }

    void RocketLauncher::on_main_window_closing()
    {
        Window& main_window{ get_windows_lib().get_main() };
    // EditorLayer
        main_window.pop_layer();
        editor_layer_ = nullptr;
    // DockSpaceLayer
        main_window.pop_overlay(); // will unregister all panels here
        dockspace_ = nullptr;
    }
}

int main(int argc, char** argv)
{
    using namespace rke;
    execute(create_scope<RocketLauncher>());
    return 0;
}
