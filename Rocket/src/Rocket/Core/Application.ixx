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
import Modal;
import ModalRegistry;
import Project;

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

        inline WindowsLib& get_windows_lib() { return windows_lib_; }
        inline Project* get_project() { return project_.get(); }
        inline void load_project(const Path& path) { project_ = Project::load_from(path); }
        inline void clear_project() { project_.reset(); }

        void register_panel(Panel* handle, PanelRegistry::Attrib attrib = {});
        void unregister_panel(Panel* handle);
        void register_modal(Modal* handle, ModalRegistry::Attrib attrib);
        void unregister_modal(Modal* handle);
    protected:
        Application();
        virtual ~Application() {};
    private:
        static void on_window_loaded(Window& window);
    private:
        WindowsLib windows_lib_;
        Scope<Project> project_{};

        PanelRegistry* panel_reg_{};
        ModalRegistry* modal_reg_{};
        ApplicationPanel panel_{ u8"Application" };
    };

    RKE_API Application& app();
    RKE_API void execute(Scope<Application> app);
}
