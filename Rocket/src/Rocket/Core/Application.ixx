module;

#include <memory>
#include "rke_macros.h"

export module Application;

import Window;
import WindowsLib;
import String;
import Path;
import Event;
import Font;
import HeapManager;
import ImGuiLayer;

export namespace rke
{
    class RKE_API Application
    {
    public:
        friend struct std::default_delete<Application>;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        void run();
        void on_event(Event& e); // for glfw(input) callbacks

        WindowsLib& get_windows_lib() const { return *(windows_.get()); }
        Window* create_window(Scope<Window::Props> props);
        const Window* get_window(const String& name) const { return (*windows_)[name]; }
        Window* get_window_mut(const String& name) { return (*windows_)[name]; }

        void remove_window(const String& window_title);
        bool single_window() const { return windows_->size() == 1; }

        ImGuiLayer* get_imgui_layer() { return imgui_layer_; }
    protected:
        Application();
        virtual ~Application() {};
    protected:
        ImGuiLayer* imgui_layer_{};
    private:
        bool on_window_closed(WindowClosedEvent& e);
        // maybe should move to WindowsLib
    private:
        Scope<WindowsLib> windows_{};
    };

    RKE_API Application& app();
    RKE_API void execute(Scope<Application> app);
}
