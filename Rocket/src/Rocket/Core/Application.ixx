module;

#include <memory>
#include "rke_macros.h"

export module Application;

import Window;
import WindowLibrary;
import Path;
import Event;
import Font;
import HeapManager;
import ImGuiLayer;

export namespace rke
{
    class RKE_API Application // Singleton
    {
    public:
        friend struct std::default_delete<Application>;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        static Application& get();

        void run();
        void on_event(Event& e); // for glfw(input) callbacks

        const Path& get_engine_assets_dir() const
            { return engine_assets_dir_; }
        Path asset_path(const Path& relative) const
            { return engine_assets_dir_ / relative; }

        Window* create_window(const Window::WindowProps& props);
        const Window* get_window(const String& title) const
            { return (*windows_)[title]; }
        Window* get_window_mut(const String& title)
            { return (*windows_)[title]; }
        WindowLibrary* get_window_lib() const { return windows_.get(); }

        void remove_window(const String& window_title);
        bool single_window() const { return windows_->size() == 1; }

    #ifndef RKE_SHIPPING
        ImGuiLayer* get_imgui_layer() { return imgui_layer_; }
    #endif
    protected:
        Application();
        virtual ~Application();
    protected:
        Path engine_assets_dir_{};
    #ifndef RKE_SHIPPING
        ImGuiLayer* imgui_layer_{}; // Only one ImGuiLayer surpported!
    #endif
    private:
        bool on_window_closed(WindowClosedEvent& e);
        // maybe should move to WindowLibrary
    private:
        Scope<WindowLibrary> windows_{};
    };

    inline void enter(Scope<Application> app) { app->run(); }
}
