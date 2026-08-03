module;

#include <utility>
#include "rke_macros.h"
namespace rke { class Window; }

export module Layer;

import Types;
import String;
import Event;
import KeyEvent;
import MouseEvent;
import ApplicationEvent;

export namespace rke
{
    class RKE_API Layer
    {
    public:
        friend class LayerStack;

        Layer(String name, Window* owner);
        virtual ~Layer() {};

        Layer(const Layer&) = delete;
        Layer(Layer&&) = delete;

        virtual void on_attach() {};
        virtual void on_detach() {};

        virtual void on_event(Event& e) = 0;
        virtual void on_update(float dt);
        virtual void on_render() {}

        virtual bool should_block_mouse() = 0;
        virtual bool should_block_keyboard() = 0;
        // depends on THIS layer

        const String& get_owner_name();
        inline StringView get_debug_name() const { return debug_name_; }
        inline Size get_index() const { return layer_index_; }
    protected:
        bool mouse_blocked() const;
        bool keyboard_blocked() const;
        // depends on FORMER layers
    protected:
        Window* owner_;
        String debug_name_;
        Size layer_index_{};
    };
}
