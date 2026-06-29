module;
module Layer;

import Log;
import Application;
import EventDispatcher;
import Window;
import Input;

namespace rke
{
    void Layer::on_update(float dt)
    {
        Input::transition_input_state
            (mouse_blocked(), keyboard_blocked());
    }

    void Layer::on_event(Event& e)
    {
        EventDispatcher dispatcher{ e };
        dispatcher.dispatch<ViewportResizedEvent>
            ([this](ViewportResizedEvent& e) { return on_viewport_resized(e); });
        if(e.belongs_to(EventCategoryInput)) {
            dispatcher.dispatch<KeyPressedEvent>
                ([this](KeyPressedEvent& e) { return on_key_pressed(e); });
            dispatcher.dispatch<MouseButtonPressedEvent>
                ([this](MouseButtonPressedEvent& e) { return on_mouse_button_pressed(e); });
            dispatcher.dispatch<MouseScrolledEvent>
                ([this](MouseScrolledEvent& e) { return on_mouse_scrolled(e); });
        }
    }

    const String& Layer::get_owner_name()
    {
        CORE_ASSERT(owner_, u8"Layer: Owner window empty!");
        return reinterpret_cast<Window*>(owner_)->get_name();
    }

    bool Layer::mouse_blocked() const
    {
        CORE_ASSERT(owner_, u8"Layer: Owner window empty!");
        return layer_index_ < reinterpret_cast<Window*>(owner_)->get_mouse_blocking_index();
    }

    bool Layer::keyboard_blocked() const
    {
        CORE_ASSERT(owner_, u8"Layer: Owner window empty!");
        return layer_index_ < reinterpret_cast<Window*>(owner_)->get_keyboard_blocking_index();
    }
}
