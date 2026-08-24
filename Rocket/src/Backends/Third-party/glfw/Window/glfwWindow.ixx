export module Window:glfw;
import :Base;

import Types;
import LayerStack;
import FrameBuffer;

namespace rke
{
    class glfwWindow : public Window
    {
    public:
        glfwWindow(String name, Scope<Props> window_props, NativeWindow shared);
        ~glfwWindow() override;

    // context related
        std::pair<int, int> get_window_pos() const override;

    // data related
        float get_vsync_extent() const override { return data_.vsync_extent; }
        float& get_vsync_extent_mut() override  { return data_.vsync_extent; }
        bool minimized() const override { return data_.minimized; }
        void update_vsync() override;
    private:
        void create_context(NativeWindow shared_context);
    private:
        // For glfwSetWindowUserPointer(only expose necessary data)
        struct WindowData
        {
            const String& name;
            Props& props;
            bool minimized{ false };
            float vsync_extent{ 1.0f };
        } data_;
    };
}
