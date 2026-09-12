module;

#include <memory>
#include <functional>
#include "rke_macros.h"

export module Application;

import Window;
import WindowsLib;
import String;
import Path;
import Event;
import Font;
import HeapManager;
import LogPanel;
import ApplicationPanel;
import ProjectSettingPanel;
import Panel;
import PanelRegistry;    
import Modal;
import ModalRegistry;
import Project;
import DockSpace;
import RenderCommand;
import Input;
import Instrumentor;

export namespace rke
{
    class RKE_API Application
    {
    public:
        friend class ApplicationPanel;

        Application();
        virtual ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        virtual void init();
        virtual void shutdown();
        void run();
        
        void send_event(Event& e);
        void load_project(const Path& path);
        void clear_project();

        inline RenderCommand& render_command() { return *render_command_; }
        inline WindowsLib& get_windows_lib() { return windows_lib_; }
        inline Project* get_project() { return project_.get(); } // can be null

        inline Input& input() { return input_; }
        inline Instrumentor& instrumentor() { return instrumentor_; }

        void register_panel(Panel* handle, PanelRegistry::Attrib attrib = {});
        void unregister_panel(Panel* handle);
        void register_modal(Modal* handle, ModalRegistry::Attrib attrib);
        void unregister_modal(Modal* handle);
    protected:
        void set_dockspace_editor_runtime(std::function<bool()> func);
    private:
        WindowsLib windows_lib_{};

        Input input_{};
        Instrumentor instrumentor_{};

        Scope<Project> project_{};
        Scope<RenderCommand> render_command_{};

        DockSpace* dockspace_{}; // owned by WindowsLib::LayerStack::DockSpaceLayer

        Scope<LogPanel> log_panel_{};
        Scope<ApplicationPanel> app_panel_{};
        Scope<ProjectSettingPanel> proj_panel_{};
    };

    RKE_API Application& app();
    RKE_API void execute(Scope<Application> app);
}
