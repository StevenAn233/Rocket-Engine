module;

#include <GLFW/glfw3.h>
#include "rke_macros.h"

export module Window:glfw;
import :Base;

import Types;
import Event;
import LayerStack;
import Context;
import FrameBuffer;

namespace rke
{
    class glfwWindow : public Window
    {
    public:
        glfwWindow(const WindowProps& props, NativeWindow shared_handle)
            { init(props, shared_handle); }
        ~glfwWindow() override { shutdown(); }

        void swap_buffers() override;

        void on_event (Event& e) override;
        void on_update(float dt) override;
        void on_render() override;

        void on_imgui_render() override;
        // have some problems with vsync setting
        // might need to put windows in multiple threads

        uint32 get_width () const override { return data_.width;  }
        uint32 get_height() const override { return data_.height; }
        const String& get_title() const override { return data_.title; }
        const Path& get_icon_path() const override { return data_.icon_path; }
        std::pair<int, int> get_window_pos() const override;

        void push_layer	  (Scope<Layer> layer  ) override;
        void push_overlay (Scope<Layer> overlay) override;
        Scope<Layer> pop_layer	(Layer* layer  ) override;
        Scope<Layer> pop_overlay(Layer* overlay) override;

        int get_mouse_blocking_index   () const override
            { return mouse_blocking_layer_index_;    }
        int get_keyboard_blocking_index() const override
            { return keyboard_blocking_layer_index_; }

        void set_event_callback(const EventCallbackFunc& callback) override
            { data_.event_callback = callback; }

        void  update_vsync() override;
        float get_vsync_extent() const override { return data_.vsync_extent; }
        float& get_vsync_extent_mut() override  { return data_.vsync_extent; }

        NativeWindow get_native_window() const override
            { return static_cast<NativeWindow>(window_); }
        bool minimized() const override { return data_.minimized; }

        void make_context_current() override;
        void check_layer_blocking() override;

        void should_close(bool judge) override { should_close_ = judge; }
        bool should_close() const override { return should_close_; }
    private:
        void init(const WindowProps& props, NativeWindow shared_handle);
        void shutdown();
    private:
        GLFWwindow* window_{};
        Scope<Context> context_{};

        Size mouse_blocking_layer_index_{};
        Size keyboard_blocking_layer_index_{};

        // For glfwSetWindowUserPointer(only expose necessary data)
        struct WindowData
        {
            String title{};
            Path icon_path{};

            uint32 width {};
            uint32 height{};

            bool minimized{ false };
            float vsync_extent{ 1.0f };
            EventCallbackFunc event_callback{};
        };

        WindowData data_{};
        bool should_close_{ false };

        LayerStack layer_stack_{};
    };
}
