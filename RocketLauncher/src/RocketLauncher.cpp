import rke;
import EditorLayer;

namespace rke
{
    class RocketLauncher : public Application
    {
    public:
        RocketLauncher() : Application()
        {
            get_windows_lib().load_main
            (
                create_scope<Window::Props>(Window::Props
                {
                    .title{ u8"Rocket" },
                    .icon_path{ file::assets_dir() / u8"icons" / u8"RKE.png" },
                    .width{ 2450 }, .height{ 1300 },
                    .x_coord{ 50 }, .y_coord{ 100 }
                })
            );
        }

        ~RocketLauncher() override {}

        void init() override
        {
            Application::init();
            Window& main_window{ get_windows_lib().get_main() };
            auto editor_layer{ create_scope<EditorLayer>(u8"EditorLayer", &main_window) };
            main_window.push_layer(std::move(editor_layer));
        }
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
