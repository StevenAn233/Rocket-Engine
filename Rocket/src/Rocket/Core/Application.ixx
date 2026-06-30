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
        void send_event(Event& e);

        Window* create_window(Scope<Window::Props> props);
        void remove_window(const String& window_title);

        inline WindowsLib& get_windows_lib() const { return *(windows_lib_.get()); }
        inline const Window* get_window(const String& name) const { return (*windows_lib_)[name]; }
        inline Window* get_window_mut(const String& name) { return (*windows_lib_)[name]; }
        inline ImGuiLayer* get_imgui_layer() { return imgui_layer_; }
    protected:
        Application();
        virtual ~Application() {};
    protected:
        ImGuiLayer* imgui_layer_{};
    private:
        bool on_window_closed(WindowClosedEvent& e);
        // maybe should move to WindowsLib
    private:
        Scope<WindowsLib> windows_lib_{};
    };

    RKE_API Application& app();
    RKE_API void execute(Scope<Application> app);
}
