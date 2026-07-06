module;
module Application;

import Log;
import String;
import Project;
import Instrumentor;
import DeltaTime;
import Renderer2D;
import PlatformSupport;
import FileUtils;
import Layer;
import DockSpaceLayer;
import HeapManager;

namespace rke
{
    Application::Application() : windows_lib_(on_window_loaded) {}

    void Application::init()
    {
        Window& main_window{ windows_lib_.get_main() };
        DockSpaceLayer* ds_layer = new DockSpaceLayer
        {
            u8"Dockspace Layer", windows_lib_.main_window_,
            file::editor_dir() / u8"settings" / u8"dockspace.yaml"
        };
        ds_handle_ = &(ds_layer->dockspace_);
        main_window.push_overlay(Scope<Layer>(static_cast<Layer*>(ds_layer)));
        get_dockspace().get_panel_registry().register_panel(&panel_);
    }

    void Application::shutdown() { ds_handle_ = nullptr; }

    void Application::run()
    {
        while(windows_lib_.valid())
        {
            RKE_PROFILE_SCOPE(u8"void Application::run(void) loop_frame");

            DeltaTime::update();
            windows_lib_.update_all(DeltaTime::get());

            Renderer2D::reset_stats();
            windows_lib_.render_all();

            windows_lib_.refresh();
        }
    }

    void Application::send_event(Event& e) { windows_lib_.on_event(e); }

    DockSpace& Application::get_dockspace()
    {
        CORE_ASSERT(ds_handle_, u8"Application: Dockspace not created!");
        return *ds_handle_;
    }

    void Application::on_window_loaded(Window& window)
        { Renderer2D::register_context(window.get_context()); }
}

namespace rke
{
    static Scope<Application> s_instance{};

    static void register_instance(Scope<Application> instance)
    {
        CORE_ASSERT(!s_instance, u8"Rocket: Instance already existed!");
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

        Project::init_file_templates(file::assets_dir() / u8"proj-templates");
        DeltaTime::update();
        DeltaTime::update();

        PlatformSupport::init();
        Renderer2D::init();

        app().init();
        app().run();
        app().shutdown();

        Renderer2D::shutdown();
        PlatformSupport::shutdown();

        unregister_instance();
    }
}
