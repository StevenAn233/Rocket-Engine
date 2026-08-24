module;
module Layer;

import Log;
import Window;
import Input;
import Application;
import EventDispatcher;

namespace rke
{
    Layer::Layer(String name, Window* owner)
        : debug_name_(std::move(name)), owner_(owner)
    { CORE_ASSERT(owner_, u8"Layer: Owner window empty!"); }

    void Layer::on_update(float dt) // only method that will write to app().input()
        { app().input().transition_input_state(mouse_blocked(), keyboard_blocked()); }

    bool Layer::mouse_blocked() const
        { return layer_index_ < owner_->get_mouse_blocking_index(); }
        
    bool Layer::keyboard_blocked() const
        { return layer_index_ < owner_->get_keyboard_blocking_index(); }
}
