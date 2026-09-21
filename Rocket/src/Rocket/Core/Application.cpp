module;
module Application;

import PlatformSupport;
import FileUtils;
import Layer;
import DockSpace;
import EventDispatcher;
import ApplicationEvent;
import ProjectEvent;

namespace rke
{
    Application::Application()
    {
        render_command_ = RenderCommand::create();
        platform_support::begin();
    }

    Application::~Application()
    {
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
                on_main_window_closing();
            return false;
        });
        windows_lib_.on_event(e);
    }

    void Application::load_project(const Path& path)
    {
        clear_project();
        project_ = create_scope<Project>(path);
        if(!project_) {
            CORE_ERROR(u8"Application: Failed to load project '{}'!", path);
            return;
        }
        ProjectLoadedEvent event{ u8"main" };
        send_event(event);

        // for calling callback; may modify
        project_->set_aa(project_->get_config().anti_aliasing);
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
}

namespace rke
{
    static LogHistory s_log_history{}; // may modify
    static Application* s_app_handle{};

    static void register_instance(Application* handle)
    {
        if(s_app_handle) DEBUG_BREAK;
        s_app_handle = handle;
    }

    static void unregister_instance() { s_app_handle = nullptr; }

    LogHistory& log_history() { return s_log_history; }

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
