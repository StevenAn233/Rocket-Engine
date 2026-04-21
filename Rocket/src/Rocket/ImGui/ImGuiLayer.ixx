module;

#include "rke_macros.h"

export module ImGuiLayer;

import String;
import Path;
import Instrumentor;
import HeapManager;
import Event;
import Layer;

export namespace rke
{
    class RKE_API ImGuiLayer : public Layer
    {
    public:
        struct StyleConfig
        {
            Path  font_path{};
            float font_size{};
        };

        ImGuiLayer(String window_title, String name, StyleConfig config);

        void on_event(Event& e) override;
        void on_attach() override;
        void on_detach() override;

        bool should_block_mouse   () override;
        bool should_block_keyboard() override;

        void begin_render();
        void end_render  ();
        void set_main_viewport_hovered(bool judge) { main_viewport_hovered_ = judge; }
        void set_main_viewport_focused(bool judge) { main_viewport_focused_ = judge; }

        bool valid() const { return valid_; }
    private:
        StyleConfig style_config_{};

        bool main_viewport_hovered_{ false };
        bool main_viewport_focused_{ false };
        bool valid_{ false };
    };
}
