import rke;
import EditorLayer;

namespace rke
{
    class RocketLauncher : public Application
    {
    public:
        RocketLauncher() : Application() {}
        ~RocketLauncher() override {}

        void init() override
        {
            Application::init();
            Window& main_window{ get_windows_lib().get_main() };
            auto editor_layer{ create_scope<EditorLayer>(u8"EditorLayer", &main_window) };
            main_window.push_layer(std::move(editor_layer));
        }

        void shutdown() override { Application::shutdown(); }
    };
}

int main(int argc, char** argv)
{
    using namespace rke;
    RKE_INFO(u8"Welcome to Rocket Engine!");
    execute(create_scope<RocketLauncher>());
    RKE_INFO(u8"Bye!");
    return 0;
}
