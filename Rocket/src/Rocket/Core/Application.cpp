module;
module Application;

import Log;
import String;
import Project;
import Instrumentor;
import DeltaTime;
import Renderer2D;
import EventDispatcher;
import PlatformSupport;
import FileUtils;

namespace rke {
    Application::Application() { windows_ = WindowsLib::create(); }

    Window* Application::create_window(Scope<Window::Props> props)
    {
        Window* window{ windows_->load(std::move(props)) };
        window->set_event_callback([this](Event& e) { on_event(e); });
        
        window->make_context_current();
        Renderer2D::register_context(window->get_native_window().val());
        return window;
    }

    void Application::on_event(Event& e)
    {
        // check window resized or closed FIRST
        EventDispatcher dispatcher{ e };
        dispatcher.dispatch<WindowClosedEvent>
            ([this](WindowClosedEvent& e) { return on_window_closed(e); });

        if(e.handled()) return; // maybe useless...

        for(auto& [_, window] : (*windows_))
            window->on_event(e);
    }

    void Application::run()
    {
        while(!windows_->is_empty())
        {
            RKE_PROFILE_SCOPE(u8"void Application::run(void) loop_frame");

            DeltaTime::update();
            for(auto& [_, window] : (*windows_))
            {
                Renderer2D::reset_stats();
                window->on_update(DeltaTime::get());
                window->on_render();
                window->on_imgui_render();
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
        remove_window(e.get_window_name());
        if(!imgui_layer_ || !imgui_layer_->valid())
            imgui_layer_ = nullptr;
        return true;
    }
}

namespace {
    using namespace rke;
    static Scope<Application> s_instance{};
}

namespace rke
{
    Application& app()
    {
        CORE_ASSERT(s_instance, u8"Rocket: Instance haven't been created!");
        return *s_instance;
    }

    void execute(Scope<Application> instance)
    {
        CORE_ASSERT(!s_instance, u8"Rocket: Instance already existed!");
        s_instance = std::move(instance);

        Project::init_file_templates(file::assets_dir() / u8"proj-templates");
        DeltaTime::update();
        DeltaTime::update();

        PlatformSupport::init();
        Renderer2D::init();

        s_instance->run();

        Renderer2D::shutdown();
        PlatformSupport::shutdown();
        s_instance.reset();
    }
}
