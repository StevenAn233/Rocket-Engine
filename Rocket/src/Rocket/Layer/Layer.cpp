module;
module Layer;

import Application;
import EventDispatcher;
import Window;
import Input;

namespace rke
{
    Layer::Layer(String window_title, String name)
        : debug_name_ (std::move(name))
        , owner_title_(std::move(window_title)) {}

    void Layer::on_attach()
        { owner_ = Application::get().get_window_mut(owner_title_); }
    void Layer::on_detach() {}

    void Layer::on_update(float dt)
        { Input::transition_input_state(mouse_blocked(), keyboard_blocked()); }

    void Layer::on_event(Event& e)
    {
        EventDispatcher dispatcher(e);

        dispatcher.dispatch<KeyPressedEvent>
            ([this](KeyPressedEvent& e) { return on_key_pressed(e); });
        dispatcher.dispatch<MouseButtonPressedEvent>
            ([this](MouseButtonPressedEvent& e) { return on_mouse_button_pressed(e); });
        dispatcher.dispatch<MouseScrolledEvent>
            ([this](MouseScrolledEvent& e) { return on_mouse_scrolled(e); });
        dispatcher.dispatch<ViewportResizedEvent>
            ([this](ViewportResizedEvent& e) { return on_viewport_resized(e); });
        dispatcher.dispatch<NewSceneEvent>
            ([this](NewSceneEvent& e) { return on_new_scene(e); });
    }

    const String& Layer::get_name() const { return debug_name_; }

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

    Window* Layer::get_owner()
    {
        CORE_ASSERT(owner_, u8"Layer: Owner window empty!");
        return owner_;
    }
}
