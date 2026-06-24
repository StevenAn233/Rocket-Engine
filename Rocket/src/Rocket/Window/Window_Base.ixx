module;

#include <memory>
#include <utility>
#include <functional>

export module Window:Base;

import Types;
import HeapManager;
import Event;
import Log;
import String;
import Path;
import Layer;
import LayerStack;

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
            String name;
            String title{ u8"Rocket Engine" };
            Path icon_path{};

            uint32 width { 1920 };
            uint32 height{ 1080 };

            uint32 x_coord{ 100 };
            uint32 y_coord{ 100 };
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

        inline uint32 get_width() const { return props_.width; }
        inline uint32 get_height() const { return props_.height; }
        inline const String& get_name() const { return props_.name; }
        inline StringView get_title() const { return props_.title; }
        inline const Path& get_icon_path() const { return props_.icon_path; }
        
        virtual std::pair<int, int> get_window_pos() const = 0;
        virtual int get_mouse_blocking_index() const = 0;
        virtual int get_keyboard_blocking_index() const = 0;

        virtual void set_event_callback(EventCallbackFunc callback) = 0;
        virtual void update_vsync() = 0;

        virtual float  get_vsync_extent() const = 0;
        virtual float& get_vsync_extent_mut() = 0;

        virtual NativeWindow get_native_window() const = 0;
        virtual bool minimized() const = 0;

        virtual void make_context_current() = 0;
        virtual void check_layer_blocking() = 0;

        inline void push_layer(Scope<Layer> layer)
            { layer_stack_.push_layer(std::move(layer)); }

        inline Scope<Layer> pop_layer(Layer* layer)
            { return layer_stack_.pop_layer(layer); }

        inline void push_overlay(Scope<Layer> overlay)
            { layer_stack_.push_overlay(std::move(overlay)); }

        inline Scope<Layer> pop_overlay(Layer* overlay)
            { return layer_stack_.pop_overlay(overlay); }

        inline void should_close(bool judge) { should_close_ = judge; }
        inline bool should_close() const { return should_close_; }

        static Scope<Window> create(const WindowProps& props, NativeWindow handle);
    protected:
        Window(WindowProps props) : props_(std::move(props)) {};
        virtual ~Window() = default;
    protected:
        WindowProps props_;
        LayerStack layer_stack_{};
    private:
        bool should_close_{ false };
    };
}
