module;
module Application;

import Log;
import Instrumentor;
import DeltaTime;
import Renderer2D;
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
    Application::Application() : windows_lib_(on_window_loaded) {}

    void Application::init()
    {
        Window& main_window{ get_windows_lib().load_main
        (
            create_scope<Window::Props>(Window::Props
            {
                .title{ u8"Rocket Engine" },
                .icon_path{ file::assets_dir() / u8"icons" / u8"RKE.png" },
                .width{ 2450 }, .height{ 1300 },
                .x_coord{ 50 }, .y_coord{ 100 }
            })
        )};

        DeltaTime::update();
        DeltaTime::update();
        PlatformSupport::init();
        Renderer2D::init();

        Scope<DockSpaceLayer> ds_layer{ create_scope<DockSpaceLayer>
        (
            u8"Dockspace Layer", windows_lib_.main_window_,
            file::editor_dir() / u8"settings" / u8"dockspace.yaml"
        )};
        dockspace_ = &(ds_layer->dockspace_);
        main_window.push_overlay(Scope<Layer>(ds_layer.release()));
        register_panel(&project_setting_panel_);
        register_panel(&application_panel_);
        register_panel(&main_window.setting_panel_);
    }

    void Application::shutdown()
    {
        Renderer2D::shutdown();
        PlatformSupport::shutdown();
    }

    void Application::run()
    {
        while(windows_lib_.valid())
        {
            RKE_PROFILE_SCOPE(u8"void Application::run(void) loop_frame");

            for(auto& [_, window] : windows_lib_.map_)
            {
                window->make_context_current();

                DeltaTime::update();
                window->on_update(DeltaTime::get());

                Renderer2D::reset_stats();
                window->on_render();
            }
            windows_lib_.refresh();
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
                unregister_panel(&application_panel_);
                unregister_panel(&project_setting_panel_);
                dockspace_->editor_runtime_ = nullptr;
                dockspace_ = nullptr;
            }
            return false;
        });
        windows_lib_.on_event(e);
    }

    void Application::load_project(const Path& path)
    {
        project_ = Project::load_from(path);
        if(project_) {
            project_setting_panel_.set_aa(project_->get_config().anti_aliasing);
            ProjectLoadedEvent event{ u8"main" };
            send_event(event);
        }
        else clear_project();
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

// callbacks
    void Application::on_window_loaded(Window& window)
        { Renderer2D::register_context(window.get_context()); }
}

namespace rke
{
    static Scope<Application> s_instance{};

    static void register_instance(Scope<Application> instance)
    {
        CORE_ASSERT(!s_instance, u8"Rocket: Instance already exists!");
        s_instance = std::move(instance);
    }

    static void unregister_instance() { s_instance.reset(); }

    Application& app()
    {
        CORE_ASSERT(s_instance, u8"Rocket: Instance haven't been created!");
        return *s_instance;
    }

    void execute(Scope<Application> instance)
    {
        register_instance(std::move(instance));
        Project::init_templates(file::assets_dir() / u8"proj-templates");
        
        app().init();
        app().run();
        app().shutdown();

        unregister_instance();
    }
}
