module;

#include <utility>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Panel;

import String;

export namespace rke
{
    class RKE_API Panel
    {
    public:
        friend class PanelRegistry;

        Panel(String name) : name_(std::move(name)) {}
        virtual ~Panel() = default;

        void refresh_state();
        virtual void on_imgui_render() = 0;

        inline const String& get_name() const { return name_; }
        inline bool on() const { return on_; }
        inline bool hidden() const { return hidden_; }
        inline bool visible() const { return size_.x > 0 && size_.y > 0; }

        inline void hide() { hidden_ = true;  }
        inline void show() { hidden_ = false; }

        inline bool is_hovered() const { return is_hovered_; }
        inline bool is_focused() const { return is_focused_; }

        inline glm::vec2 get_size() const { return size_; }
        inline glm::vec2 get_abs_pos() const { return abs_pos_; }
        inline glm::vec2 get_abs_mouse_pos() const { return abs_mouse_pos_; }
        inline glm::vec2 get_mouse_pos() const { return mouse_pos_; }

        inline bool resized() const { return resized_; }
        inline bool relocated() const { return relocated_; }
    private:
        String name_;
        
        glm::vec2 size_{ 0.0f };
        glm::vec2 abs_pos_{ 0.0f };
        glm::vec2 abs_mouse_pos_{ 0.0f };
        glm::vec2 mouse_pos_{ 0.0f };
        bool resized_{ false };
        bool relocated_{ false };

        bool on_{ true };
        bool hidden_{ false };
        bool is_hovered_{ false };
        bool is_focused_{ false };
    };
}
