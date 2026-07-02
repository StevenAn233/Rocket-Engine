module;
module Window;

namespace rke
{
    void Window::check_layer_blocking()
    {
        mouse_blocking_layer_index_	= 0;
        keyboard_blocking_layer_index_ = 0;

        for(auto it{ layer_stack_.rbegin() }; it != layer_stack_.rend(); ++it)
        {
            Layer& layer{ *(it->get()) };
            if(layer.should_block_mouse())
                { mouse_blocking_layer_index_ = layer.get_index(); return; }
            if(layer.should_block_keyboard())
                { keyboard_blocking_layer_index_ = layer.get_index(); return; }
        }
    }

    void Window::on_event(Event& e)
    {
        if(name_ != e.get_window_name()) return;
        for(auto it{ layer_stack_.rbegin() }; it != layer_stack_.rend(); ++it)
        {
            if(e.handled()) return;
            it->get()->on_event(e);
            // if upper layer(overlays first) has handled event
            // then break(do not let other layers deal with it)
        }
    }

    void Window::on_update(float dt)
    {
        check_layer_blocking();
        make_context_current();
        for(const auto& layer : layer_stack_)
            layer->on_update(dt);
    }

    void Window::on_render()
    {
        if(minimized()) return;
        for(auto it{ layer_stack_.rbegin() }; it < layer_stack_.rend(); ++it)
            it->get()->on_render();
    }

    void Window::on_imgui_render()
    {
        for(auto it{ layer_stack_.rbegin() }; it < layer_stack_.rend(); ++it)
            it->get()->on_imgui_render();
    }
}

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke
{
    Scope<Window> Window::create(String name,
        Scope<Props> props, NativeWindow handle)
    {
        return create_scope<glfwWindow>
            (std::move(name), std::move(props), handle);
    }
}
#endif
