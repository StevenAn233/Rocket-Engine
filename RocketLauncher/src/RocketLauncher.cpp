import rke;
import EditorLayer;

namespace rke
{
    class RocketLauncher : public Application
    {
    public:
        RocketLauncher() : Application()
        {
            Window* main_window{ create_window
            ({
                .width{ 2450 }, .height{ 1300 },
                .x_coord{ 50 }, .y_coord{ 100 },
                .icon_path{ engine_assets_dir_ / u8"icons" / u8"RKE.png" }
            })};

        #ifndef RKE_SHIPPING
            ImGuiLayer::StyleConfig config{};
            Path config_path{ file::find_editor_dir() / u8"config" / u8".style" };

            auto cr{ ConfigReader::create(config_path) };
            auto font_data{ cr ? cr->get_child(u8"Font") : nullptr };
            if(font_data) {
                config.font_path = file::unify_path(font_data->get_at(u8"Path", String{}));
                config.font_size = font_data->get_at(u8"Size", 1.0f);
            }

            auto imgui_layer{ create_scope<ImGuiLayer>
                (main_window->get_title(), u8"ImGuiLayer", config) };
            imgui_layer_ = imgui_layer.get();
            main_window->push_overlay(std::move(imgui_layer));

            auto editor_layer{ create_scope<EditorLayer>
                (main_window->get_title(), u8"EditorLayer") };
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

    enter(create_scope<RocketLauncher>());

    RKE_INFO(u8"Bye.");
    return 0;
}
