module;

#include <utility>
#include "rke_macros.h"

export module Layer;

import Types;
import String;
import Event;

export namespace rke
{
    class RKE_API Layer
    {
    public:
        friend class LayerStack;

        Layer(String name, void* owner)
            : debug_name_(std::move(name)), owner_(owner) {}
        Layer(const Layer&) = delete;
        Layer(Layer&&) = delete;
        virtual ~Layer() {}

        virtual void on_attach() {};
        virtual void on_detach() {};

        virtual void on_update(float dt);

        virtual void on_render() {}
        virtual void on_imgui_render() {}

        virtual bool should_block_mouse() { return false; }
        virtual bool should_block_keyboard() { return false; }
        // depends on THIS layer

        virtual void on_event(Event& e);

        inline StringView get_debug_name() const { return debug_name_; }
        const String& get_owner_name();
        inline Size get_index() const { return layer_index_; }
    protected:
        bool mouse_blocked() const;
        bool keyboard_blocked() const;
        // depends on FORMER layers

        virtual bool on_key_pressed(KeyPressedEvent& e) { return should_block_keyboard(); }
        virtual bool on_mouse_button_pressed(MouseButtonPressedEvent& e) { return should_block_mouse(); }
        virtual bool on_mouse_scrolled(MouseScrolledEvent& e) { return should_block_mouse(); }
        virtual bool on_viewport_resized(ViewportResizedEvent& e) { return false; } // do not block
    protected:
        void* owner_{};
        String debug_name_;
        Size layer_index_{};
    };
}
