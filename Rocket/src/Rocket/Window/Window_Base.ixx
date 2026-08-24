module;

#include <memory>
#include <utility>
#include <functional>
#include "rke_macros.h"

export module Window:Base;

import Log;
import Time;
import Types;
import HeapManager;
import Event;
import String;
import Path;
import Layer;
import LayerStack;
import NativeWindow;
import WindowSettingPanel;
import Renderer2D;

export namespace rke
{
    class RKE_API Window
    {
    public:
        friend class Application;
        friend class WindowsLib;

        struct Props
        {
            String title{ u8"Rocket Engine" };
            Path icon_path{};

            uint32 width { 1920 };
            uint32 height{ 1080 };

            uint32 x_coord{ 100 };
            uint32 y_coord{ 100 };
        };
    public:        
        Window(String name, Scope<Props> props);
        virtual ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        inline const String& get_name() const { return name_; }
        inline NativeWindow get_context() const { return context_; }
        inline uint32 get_width() const { return props_->width; }
        inline uint32 get_height() const { return props_->height; }
        inline StringView get_title() const { return props_->title; }
        inline const Path& get_icon_path() const { return props_->icon_path; }
        inline double get_last_elapsed() const { return timer_.get_last_elapsed(); }

        inline Renderer2D& renderer_2d() { return *renderer_2d_; }

        inline Size get_mouse_blocking_index() const
            { return mouse_blocking_layer_index_; }
        inline Size get_keyboard_blocking_index() const
            { return keyboard_blocking_layer_index_; }
        
        inline void push_layer(Scope<Layer> layer)
            { layer_stack_.push_layer(std::move(layer)); }
        inline void push_overlay(Scope<Layer> overlay)
            { layer_stack_.push_overlay(std::move(overlay)); }
        inline Scope<Layer> pop_layer()
            { return layer_stack_.pop_layer(); }
        inline Scope<Layer> pop_overlay()
            { return layer_stack_.pop_overlay(); }
        inline Scope<Layer> pop_back()
            { return layer_stack_.pop_back(); }

        inline void should_close(bool judge) { should_close_ = judge; }
        inline bool should_close() const { return should_close_; }

        void check_layer_blocking();
        void make_context_current() const;

    // context related
        virtual std::pair<int, int> get_window_pos() const = 0;

    // data related
        virtual float get_vsync_extent() const = 0;
        virtual float& get_vsync_extent_mut() = 0;
        virtual bool minimized() const = 0;
        virtual void update_vsync() = 0;

        static Scope<Window> create(String name,
            Scope<Props> props, NativeWindow context);
    private:
        void on_event(Event& e);
        void on_update();
        void on_render();
    protected:
        NativeWindow context_{};
        Scope<Props> props_;
        Scope<Renderer2D> renderer_2d_{};
    private:
        String name_;
        Timer timer_;
        Ticker ticker_;
        bool should_close_{ false };

        LayerStack layer_stack_{};
        Size mouse_blocking_layer_index_{};
        Size keyboard_blocking_layer_index_{};

        WindowSettingPanel setting_panel_;
    };
}
