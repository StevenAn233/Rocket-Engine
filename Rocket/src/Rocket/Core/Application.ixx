module;

#include <memory>
#include "rke_macros.h"

export module Application;

import Window;
import WindowsLib;
import String;
import Path;
import Event;
import ApplicationEvent;
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

        WindowsLib& get_windows_lib() { return windows_lib_; }
    // TO REMOVE
        inline ImGuiLayer* get_imgui_layer() { return imgui_layer_; }
    protected:
        Application();
        virtual ~Application() {};
    protected:
    // TO REMOVE
        ImGuiLayer* imgui_layer_{};
    private:
        static void on_window_loaded(Window& window);
    private:
        WindowsLib windows_lib_;
    };

    RKE_API Application& app();
    RKE_API void execute(Scope<Application> app);
}
