module;

#include <utility>
#include "rke_macros.h"

export module Panel;

import String;

export namespace rke
{
    class RKE_API Panel
    {
    public:
        friend class PanelRegistry;

        virtual ~Panel() = default;

        virtual void on_imgui_render() { refresh_state(); }

        inline const String& get_name() const { return name_; }
        inline bool on() const { return on_; }
        inline bool hidden() const { return hidden_; }

        inline void hide() { hidden_ = true;  }
        inline void show() { hidden_ = false; }

        bool is_hovered() const { return is_hovered_; }
        bool is_focused() const { return is_focused_; }
    protected:
        Panel(String name) : name_(std::move(name)) {}
    private:
        inline void render() { if(on_ && !hidden_) on_imgui_render(); }
        void refresh_state();
    private:
        String name_;
        bool on_{ true };
        bool hidden_{ false };
        bool is_hovered_{ false };
        bool is_focused_{ false };
    };
}
