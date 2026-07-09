module;
module Application;

import Log;
import Project;
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

        DockSpaceLayer* ds_layer{ new DockSpaceLayer
        {
            u8"Dockspace Layer", windows_lib_.main_window_,
            file::editor_dir() / u8"settings" / u8"dockspace.yaml"
        }};
        panel_reg_ = &(ds_layer->dockspace_.get_panel_registry());
        modal_reg_ = &(ds_layer->dockspace_.get_modal_registry());
        main_window.push_overlay(Scope<Layer>(static_cast<Layer*>(ds_layer)));
        register_panel(&panel_);
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

            DeltaTime::update();
            windows_lib_.update_all(DeltaTime::get());

            Renderer2D::reset_stats();
            windows_lib_.render_all();

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
                unregister_panel(&panel_);
                panel_reg_ = nullptr;
                modal_reg_ = nullptr;
            }
            return false;
        });
        windows_lib_.on_event(e);
    }

    void Application::register_panel(Panel* handle, PanelRegistry::Attrib attrib)
    {
        if(!panel_reg_) return;
        panel_reg_->register_panel(handle, attrib);
    }

    void Application::unregister_panel(Panel* handle)
    {
        if(!panel_reg_) return;
        panel_reg_->unregister_panel(handle);
    }

    void Application::register_modal(Modal* handle, ModalRegistry::Attrib attrib)
    {
        if(!modal_reg_) return;
        modal_reg_->register_modal(handle, attrib);
    }

    void Application::unregister_modal(Modal* handle)
    {
        if(!modal_reg_) return;
        modal_reg_->unregister_modal(handle);
    }

// callbacks
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
        
        app().init();
        app().run();
        app().shutdown();

        unregister_instance();
    }
}
