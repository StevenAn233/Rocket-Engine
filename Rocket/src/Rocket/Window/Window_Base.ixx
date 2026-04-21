module;

#include <memory>
#include <utility>
#include <functional>

export module Window:Base;

import Types;
import HeapManager;
import Layer;
import Event;
import Log;
import String;
import Path;

export import NativeWindow;

export namespace rke
{
    class Window
    {
    public:
        friend struct std::default_delete<Window>;

        using EventCallbackFunc = std::function<void(Event&)>;

        struct WindowProps
        {
            String title{ u8"Rocket Engine" };

            uint32 width { 1920 };
            uint32 height{ 1080 };

            uint32 x_coord{ 100 };
            uint32 y_coord{ 100 };

            Path icon_path{};
        };

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        virtual void swap_buffers() = 0;

        virtual void on_event (Event& e) = 0;
        virtual void on_update(float dt) = 0;
        virtual void on_render() = 0;

        virtual void on_imgui_render() = 0;

        virtual uint32 get_width () const = 0;
        virtual uint32 get_height() const = 0;
        virtual const Path& get_icon_path() const = 0;
        virtual const String& get_title() const = 0;
        virtual std::pair<int, int> get_window_pos() const = 0;

        virtual void push_layer	  (Scope<Layer> layer  ) = 0;
        virtual void push_overlay (Scope<Layer> overlay) = 0;
        virtual Scope<Layer> pop_layer	(Layer* layer  ) = 0;
        virtual Scope<Layer> pop_overlay(Layer* overlay) = 0;

        virtual int get_mouse_blocking_index   () const = 0;
        virtual int get_keyboard_blocking_index() const = 0;

        virtual void set_event_callback(const EventCallbackFunc& callback) = 0;
        virtual void update_vsync() = 0;

        virtual float  get_vsync_extent() const = 0;
        virtual float& get_vsync_extent_mut ()  = 0;

        virtual NativeWindow get_native_window() const = 0;
        virtual bool minimized() const = 0;

        virtual void make_context_current() = 0;
        virtual void check_layer_blocking() = 0;

        virtual void should_close(bool judge) = 0;
        virtual bool should_close() const = 0;

        static Scope<Window> create(const WindowProps& props, NativeWindow handle);
    protected:
        Window() = default;
        virtual ~Window() = default;
    };
}
