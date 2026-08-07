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
            Scope<EditorLayer> editor_layer {
                create_scope<EditorLayer>(u8"EditorLayer", &main_window) };
            editor_layer_ = editor_layer.get();
            main_window.push_layer(Scope<Layer>(editor_layer.release()));
            set_dockspace_editor_runtime([this]() { return editor_layer_->testing(); });
        }

        void shutdown() override
        {
            editor_layer_ = nullptr;
            Application::shutdown();
        }
    private:
        EditorLayer* editor_layer_{};
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
