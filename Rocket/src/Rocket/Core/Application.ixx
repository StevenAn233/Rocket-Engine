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
import ApplicationPanel;
import DockSpace;
import DockSpaceLayer;

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

        virtual void init();
        virtual void shutdown();
        void run();
        
        void send_event(Event& e);

        WindowsLib& get_windows_lib() { return windows_lib_; }
        DockSpace& get_dockspace();
    protected:
        Application();
        virtual ~Application() {};
    private:
        static void on_window_loaded(Window& window);
    private:
        WindowsLib windows_lib_;
        DockSpace* ds_handle_{};
        ApplicationPanel panel_{ u8"Application" };
    };

    RKE_API Application& app();
    RKE_API void execute(Scope<Application> app);
}
