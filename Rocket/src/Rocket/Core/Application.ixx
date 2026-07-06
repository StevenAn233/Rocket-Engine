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
import ApplicationPanel;
import Panel;
import PanelRegistry;

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

        void register_panel(Panel* handle, PanelRegistry::Attrib attrib = {});
        void unregister_panel(Panel* handle);
    protected:
        Application();
        virtual ~Application() {};
    private:
        static void on_window_loaded(Window& window);
    private:
        WindowsLib windows_lib_;
        PanelRegistry* panel_reg_{};
        ApplicationPanel panel_{ u8"Application" };
    };

    RKE_API Application& app();
    RKE_API void execute(Scope<Application> app);
}
