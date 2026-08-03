module;
module Layer;

import Log;
import Application;
import EventDispatcher;
import Window;
import Input;

namespace rke
{
    Layer::Layer(String name, Window* owner)
        : debug_name_(std::move(name)), owner_(owner) {}

    void Layer::on_update(float dt)
    {
        Input::transition_input_state
            (mouse_blocked(), keyboard_blocked());
    }

    const String& Layer::get_owner_name()
    {
        CORE_ASSERT(owner_, u8"Layer: Owner window empty!");
        return owner_->get_name();
    }

    bool Layer::mouse_blocked() const
    {
        CORE_ASSERT(owner_, u8"Layer: Owner window empty!");
        return layer_index_ < owner_->get_mouse_blocking_index();
    }

    bool Layer::keyboard_blocked() const
    {
        CORE_ASSERT(owner_, u8"Layer: Owner window empty!");
        return layer_index_ < owner_->get_keyboard_blocking_index();
    }
}
