module;
module Window;

import WindowsLib;
import Application;

namespace rke
{
    Window::Window(String name, Scope<Props> props)
        : name_(std::move(name)), props_(std::move(props))
    {
        setting_panel_.set_context(this);
        app().register_panel(&setting_panel_);
    }

    Window::~Window() { app().unregister_panel(&setting_panel_); }

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

    void Window::make_context_current() const
        { WindowsLib::make_context_current(get_context()); }

    void Window::on_event(Event& e)
    {
        if(name_ != e.get_window_name()) return;
        for(auto it{ layer_stack_.rbegin() }; it != layer_stack_.rend(); ++it)
        {
            if(e.handled()) return;
            it->get()->on_event(e);
        }
    }

    void Window::on_update(float dt)
    {
        check_layer_blocking();
        make_context_current();
        for(auto it{ layer_stack_.rbegin() }; it < layer_stack_.rend(); ++it)
            it->get()->on_update(dt);
    }

    void Window::on_render()
    {
        if(minimized()) return;
        for(const auto& layer : layer_stack_)
            layer->on_render();
    }
}

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke
{
    Scope<Window> Window::create(String name,
        Scope<Props> props, NativeWindow context)
    {
        return create_scope<glfwWindow>
            (std::move(name), std::move(props), context);
    }
}
#endif
