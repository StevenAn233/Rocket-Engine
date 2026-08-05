module;

#include <utility>
#include <functional>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module DockSpace;

import String;
import Path;
import Window;
import NativeWindow;
import PanelRegistry;
import ModalRegistry;
import ProjectCreatingModal;
import KeyEvent;

export namespace rke
{
    class RKE_API DockSpace
    {
    public:
        friend class Application;
        friend class DockSpaceLayer;

        DockSpace(String name, Path config_path, NativeWindow context);
        ~DockSpace();

        void render(glm::vec2 offset, glm::vec2 scale);
        inline PanelRegistry& get_panel_registry() { return panel_registry_; }
        inline ModalRegistry& get_modal_registry() { return modal_registry_; }
    private:
        bool should_block_mouse() const;
        bool should_block_keyboard() const;

        void on_update();
        bool on_key_pressed(KeyPressedEvent& e);

        void create_project();
        void open_project(const Window& window);
        void save_project();
    private:
        String name_;
        Path config_path_;
        uint32 flags_{};

        PanelRegistry panel_registry_{};
        ModalRegistry modal_registry_{};

        ProjectCreatingModal project_creating_modal_{ u8"Create New Project" };
        bool to_create_project_{ false };
        bool ctrl_pressed_{ false };

        std::function<bool()> editor_runtime_{};
    };
}
