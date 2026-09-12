module;
module Application;

import Log;
import PlatformSupport;
import FileUtils;
import Layer;
import DockSpace;
import DockSpaceLayer;
import EventDispatcher;
import ApplicationEvent;
import ProjectEvent;

namespace rke
{
    Application::Application() {}
    Application::~Application() {}

    void Application::init()
    {
        render_command_ = RenderCommand::create();
        platform_support::begin();

        Window& main_window{ windows_lib_.load_main
        (
            create_scope<Window::Props>(Window::Props
            {
                .title{ u8"Rocket Engine" },
                .icon_path{ file::assets_dir() / u8"icons" / u8"RKE.png" },
                .width{ 2450 }, .height{ 1300 },
                .x_coord{ 50 }, .y_coord{ 100 }
            })
        )};
        
        Scope<DockSpaceLayer> ds_layer{ create_scope<DockSpaceLayer>
        (
            u8"Dockspace Layer", &main_window,
            file::editor_dir() / u8"settings" / u8"dockspace.yaml"
        )};
        dockspace_ = &(ds_layer->dockspace_);
        main_window.push_overlay(Scope<Layer>(ds_layer.release()));

        log_panel_  = create_scope<LogPanel>(u8"Log");
        app_panel_  = create_scope<ApplicationPanel>(u8"Application");
        proj_panel_ = create_scope<ProjectSettingPanel>(u8"Project Settings");
        log_panel_->load_from(file::editor_dir() / u8"settings" / u8"log.yaml");

        register_panel(log_panel_ .get());
        register_panel(app_panel_ .get());
        register_panel(proj_panel_.get());
        register_panel(&main_window.setting_panel_);
    }

    void Application::shutdown()
    {
        log_panel_ .reset();
        app_panel_ .reset();
        proj_panel_.reset();

        platform_support::end();
        render_command_.reset();
    }

    void Application::run()
    {
        while(windows_lib_.valid())
        {
            RKE_PROFILE_SCOPE(u8"void Application::run(void) loop_frame");
            windows_lib_.loop();
        }
    }

    void Application::send_event(Event& e)
    {
        EventDispatcher(e).dispatch<WindowClosedEvent>
        ([this](WindowClosedEvent& e)
        {
            if(e.get_window_name() == u8"main")
            {
                unregister_panel(&windows_lib_.get_main().setting_panel_);
                unregister_panel(proj_panel_.get());
                unregister_panel(app_panel_ .get());
                unregister_panel(log_panel_ .get());
                dockspace_->editor_runtime_ = nullptr;
                dockspace_ = nullptr;
            }
            return false;
        });
        windows_lib_.on_event(e);
    }

    void Application::load_project(const Path& path)
    {
        clear_project();
        project_ = create_scope<Project>(path);
        if(project_) {
            proj_panel_->set_aa(project_->get_config().anti_aliasing);
            ProjectLoadedEvent event{ u8"main" };
            send_event(event);
        }
        else CORE_ERROR(u8"Application: Failed to load project '{}'!", path);
    }

    void Application::clear_project()
    {
        project_.reset();
        ProjectClearedEvent event{ u8"main" };
        send_event(event);
    }

    void Application::register_panel(Panel* handle, PanelRegistry::Attrib attrib)
    {
        if(!dockspace_) return;
        dockspace_->get_panel_registry().register_panel(handle, attrib);
    }

    void Application::unregister_panel(Panel* handle)
    {
        if(!dockspace_) return;
        dockspace_->get_panel_registry().unregister_panel(handle);
    }

    void Application::register_modal(Modal* handle, ModalRegistry::Attrib attrib)
    {
        if(!dockspace_) return;
        dockspace_->get_modal_registry().register_modal(handle, attrib);
    }

    void Application::unregister_modal(Modal* handle)
    {
        if(!dockspace_) return;
        dockspace_->get_modal_registry().unregister_modal(handle);
    }

    void Application::set_dockspace_editor_runtime(std::function<bool()> func)
        { dockspace_->editor_runtime_ = std::move(func); }
}

namespace rke
{
    LogHistory log_history{}; // may modify
    static Application* s_app_handle{};

    static void register_instance(Application* handle)
    {
        if(s_app_handle) DEBUG_BREAK;
        s_app_handle = handle;
    }

    static void unregister_instance() { s_app_handle = nullptr; }

    Application& app()
    {
        if(s_app_handle == nullptr) DEBUG_BREAK;
        return *s_app_handle;
    }

    void execute(Scope<Application> instance)
    {
        register_instance(instance.get()); // ownership still within function scope

        Project::init_templates(file::assets_dir() / u8"proj-templates");
        instance->init();
        instance->run();
        instance->shutdown();

        unregister_instance();
    }
}
