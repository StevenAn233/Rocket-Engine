module;
module Application;

import Log;
import String;
import Project;
import Instrumentor;
import DeltaTime;
import Renderer2D;
import Renderer;
import EventDispatcher;
import PlatformSupport;
import FileUtils;

namespace rke
{
    static Application* s_instance{ nullptr };

    Application::Application()
    {
        CORE_ASSERT(!s_instance, u8"Application: Instance already existed!");
        s_instance = this;

        engine_assets_dir_ = file::find_assets_dir();
        CORE_ASSERT(engine_assets_dir_.exists(), u8"Application: "
            u8"Can't locate engine assets directory '{}'!", engine_assets_dir_);

        windows_ = WindowLibrary::create();

        Project::init_file_templates(asset_path(u8"proj-templates"));
        DeltaTime::update();
        DeltaTime::update();

        PlatformSupport::init();
        Renderer2D::init();
        Renderer::init();
    }

    Application::~Application()
    {
        Renderer::shutdown();
        Renderer2D::shutdown();
        PlatformSupport::shutdown();
        s_instance = nullptr;
    }

    Application& Application::get()
    {
        CORE_ASSERT(s_instance, u8"Application: Instance haven't been created!");
        return *s_instance;
    }

    Window* Application::create_window(const Window::WindowProps& props)
    {
        windows_->load(props);
        Window* window{ (*windows_)[props.title] };
        window->set_event_callback([this](Event& e) { on_event(e); });

        window->make_context_current();
        Renderer2D::register_context();
        return window;
    }

    void Application::on_event(Event& e)
    {
        // check window resized or closed FIRST
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowClosedEvent>
            ([this](WindowClosedEvent& e) { return on_window_closed(e); });

        if(e.handled()) return; // maybe useless...

        for(auto& [title, window] : windows_->get_mut())
            window->on_event(e);
    }

    void Application::run()
    {
        while(!windows_->is_empty())
        {
            RKE_PROFILE_SCOPE(u8"void Application::run(void) loop_frame");

            DeltaTime::update();

            for(auto& [title, window] : windows_->get_mut())
            {
            #ifndef RKE_SHIPPING
                if(imgui_layer_) imgui_layer_->begin_render();
            #endif
                Renderer2D::reset_stats();
                window->on_update(DeltaTime::get());
                window->on_render();
                window->on_imgui_render();
            #ifndef RKE_SHIPPING
                if(imgui_layer_) imgui_layer_->end_render();
            #endif
            }
            windows_->refresh();
        }
    }

    void Application::remove_window(const String& window_title)
    {
        CORE_ASSERT(windows_->exists(window_title),
            u8"Application: You didn't push this window to WindowsLib?");
        windows_->remove(window_title);
    }

    bool Application::on_window_closed(rke::WindowClosedEvent& e)
    {
        CORE_INFO(e);
        remove_window(e.get_window_title());
    #ifndef RKE_SHIPPING
        if(!imgui_layer_ || !imgui_layer_->valid())
            imgui_layer_ = nullptr;
    #endif
        return true;
    }
}
