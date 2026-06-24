module;

#include <utility>
#include <GLFW/glfw3.h>
#include "rke_macros.h"

export module Window:glfw;
import :Base;

import Types;
import LayerStack;
import Context;
import FrameBuffer;

namespace rke
{
    class glfwWindow : public Window
    {
    public:
        glfwWindow(WindowProps props, NativeWindow shared_handle);
        ~glfwWindow() override;

        void swap_buffers() override;

        void on_event (Event& e) override;
        void on_update(float dt) override;
        void on_render() override;

        void on_imgui_render() override;
        // have some problems with vsync setting
        // might need to put windows in multiple threads

        std::pair<int, int> get_window_pos() const override;
        int get_mouse_blocking_index() const override { return mouse_blocking_layer_index_; }
        int get_keyboard_blocking_index() const override { return keyboard_blocking_layer_index_; }

        void set_event_callback(EventCallbackFunc callback) override
            { data_.event_callback = std::move(callback); }

        void update_vsync() override;
        float get_vsync_extent() const override { return data_.vsync_extent; }
        float& get_vsync_extent_mut() override  { return data_.vsync_extent; }

        NativeWindow get_native_window() const override
            { return static_cast<NativeWindow>(handle_); }
        bool minimized() const override { return data_.minimized; }

        void make_context_current() override;
        void check_layer_blocking() override;
    private:
        void init(NativeWindow shared_handle);
        void shutdown();
    private:
        GLFWwindow* handle_{};
        Scope<Context> context_{};

        Size mouse_blocking_layer_index_{};
        Size keyboard_blocking_layer_index_{};

        // For glfwSetWindowUserPointer(only expose necessary data)
        struct WindowData
        {
            WindowProps& props;
            bool minimized{ false };
            float vsync_extent{ 1.0f };
            EventCallbackFunc event_callback{};
        };

        WindowData data_;
    };
}
