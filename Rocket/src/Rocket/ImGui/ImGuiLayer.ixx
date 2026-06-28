module;

#include <utility>
#include "rke_macros.h"

export module ImGuiLayer;

import String;
import Path;
import Instrumentor;
import HeapManager;
import Event;
import Layer;
import Window;

export namespace rke
{
    class RKE_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer(String name, Window* owner)
            : Layer(std::move(name), owner) {}

        void on_event(Event& e) override;
        void on_attach() override;
        void on_detach() override;

        bool should_block_mouse() override;
        bool should_block_keyboard() override;

        void begin_render() const;
        void end_render() const;
        inline void set_main_viewport_hovered(bool judge) { main_viewport_hovered_ = judge; }
        inline void set_main_viewport_focused(bool judge) { main_viewport_focused_ = judge; }

        inline bool valid() const { return valid_; }
    private:
        bool main_viewport_hovered_{ false };
        bool main_viewport_focused_{ false };
        bool valid_{ false };
    };
}
