module;

#include "rke_macros.h"

export module Layer;

import Types;
import String;
import Log;
import Event;
import HeapManager;

export namespace rke
{
    class RKE_API Layer
    {
    public:
        friend class LayerStack;
        friend class Window;

        Layer(String window_title, String name);
        Layer(const Layer&) = delete;
        Layer(Layer&& ____) = delete;
        virtual ~Layer() {}

        virtual void on_attach();
        virtual void on_detach();

        virtual void on_update(float dt);

        virtual void on_render() {}
        virtual void on_imgui_render() {}

        virtual bool should_block_mouse   () { return false; }
        virtual bool should_block_keyboard() { return false; }
        // depends on THIS layer

        virtual void on_event(Event& e);

        const String& get_name() const; // can't be inline
        Size get_index() const { return layer_index_; }
    protected:
        bool mouse_blocked   () const;
        bool keyboard_blocked() const;
        // depends on FORMER layers

        virtual bool on_key_pressed(KeyPressedEvent& e)
            { return should_block_keyboard(); }
        virtual bool on_mouse_button_pressed(MouseButtonPressedEvent& e)
            { return should_block_mouse(); }
        virtual bool on_mouse_scrolled(MouseScrolledEvent& e)
            { return should_block_mouse(); }
        virtual bool on_viewport_resized(ViewportResizedEvent& e)
            { return false; } // do not block
        virtual bool on_new_scene(NewSceneEvent& e)
            { return false; } // do not block

        const String& get_owner_title() { return owner_title_; }
        Window* get_owner();
    private:
        String owner_title_;
        Window* owner_{};

        String debug_name_;
        Size layer_index_{};
    };
}
