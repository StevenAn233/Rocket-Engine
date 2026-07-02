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
    Application::Application() : windows_lib_(on_window_loaded) {}

    void Application::run()
    {
        while(!windows_lib_.empty())
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

    void Application::on_window_loaded(Window& window)
        { Renderer2D::register_context(window.get_context()); }
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
