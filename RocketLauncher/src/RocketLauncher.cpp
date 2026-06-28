import rke;
import EditorLayer;

namespace rke
{
    class RocketLauncher : public Application
    {
    public:
        RocketLauncher() : Application()
        {
            auto props{ create_scope<Window::Props>
            ( Window::Props {
                .name{ u8"main" }, .title{ u8"Rocket" },
                .icon_path{ file::assets_dir() / u8"icons" / u8"RKE.png" },
                .width{ 2450 }, .height{ 1300 },
                .x_coord{ 50 }, .y_coord{ 100 }
            }) };
            Window* main_window{ create_window(std::move(props)) };

        #ifndef RKE_SHIPPING
            auto imgui_layer{ create_scope<ImGuiLayer>(u8"ImGuiLayer", main_window) };
            imgui_layer_ = imgui_layer.get();
            main_window->push_overlay(std::move(imgui_layer));

            auto editor_layer{ create_scope<EditorLayer>(u8"EditorLayer", main_window) };
            main_window->push_layer(std::move(editor_layer));
        #endif
        }
        ~RocketLauncher() override = default;
    };
}

int main(int argc, char** argv)
{
    using namespace rke;

    RKE_INFO(u8"Welcome to Rocket Engine!" );
    RKE_INFO(u8"Executable: '{}'.", argv[0]);

    Path working_dir{ fs::current_path() };
    RKE_INFO(u8"Working Dir: '{}'.", working_dir);

    execute(create_scope<RocketLauncher>());

    RKE_INFO(u8"Bye.");
    return 0;
}
