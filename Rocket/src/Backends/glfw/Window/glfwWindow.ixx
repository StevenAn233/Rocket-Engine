module;

#include <utility>
#include <GLFW/glfw3.h>
#include "rke_macros.h"

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
        void make_context_current() override;
        void swap_buffers() override;
        std::pair<int, int> get_window_pos() const override;

    // data related
        float get_vsync_extent() const override { return data_.vsync_extent; }
        float& get_vsync_extent_mut() override  { return data_.vsync_extent; }
        void set_event_callback(EventCallbackFunc callback) override
            { data_.event_callback = std::move(callback); }
        bool minimized() const override { return data_.minimized; }
        void update_vsync() override;
    private:
        // For glfwSetWindowUserPointer(only expose necessary data)
        struct WindowData
        {
            const String& name;
            Props& props;
            bool minimized{ false };
            float vsync_extent{ 1.0f };
            EventCallbackFunc event_callback{};
        } data_;
    };
}
