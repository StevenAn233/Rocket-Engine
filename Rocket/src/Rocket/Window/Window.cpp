module;
module Window;

import WindowsLib;
import Application;

namespace rke
{
    Window::Window(String name, Scope<Props> props)
        : name_(std::move(name)), props_(std::move(props))
        , setting_panel_(name_, this)
    {
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
            if(layer.should_block_mouse()) {
                mouse_blocking_layer_index_ = layer.get_index();
                break;
            }
        }
        for(auto it{ layer_stack_.rbegin() }; it != layer_stack_.rend(); ++it)
        {
            Layer& layer{ *(it->get()) };
            if(layer.should_block_keyboard()) {
                keyboard_blocking_layer_index_ = layer.get_index();
                break;
            }
        }
    }

    void Window::make_context_current() const
        { WindowsLib::make_context_current(get_context()); }

    void Window::on_event(Event& e)
    {
        for(auto it{ layer_stack_.rbegin() }; it != layer_stack_.rend(); ++it)
        {
            if(e.handled()) return;
            it->get()->on_event(e);
        }
    }

    void Window::on_update()
    {
        check_layer_blocking();
        if(!timer_) timer_ = create_scope<Timer>();
        timer_->update();
        update_smoothed_fps();
        for(auto it{ layer_stack_.rbegin() }; it < layer_stack_.rend(); ++it)
            it->get()->on_update(get_last_elapsed());
    }

    void Window::on_render()
    {
        if(minimized()) return;
        renderer_2d().reset_stats();
        for(const auto& layer : layer_stack_)
            layer->on_render();
    }

    void Window::update_smoothed_fps()
    {
        constexpr float alpha{ 5.0f };

        double dt{ get_last_elapsed() };
        double current_fps{ 1.0 / dt };

        double lerp_alpha{ glm::clamp(dt * alpha, 0.0, 1.0) };
        smoothed_fps_ = glm::mix(smoothed_fps_, current_fps, lerp_alpha);
    }
}

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke
{
    Scope<Window> Window::create(String name,
        Scope<Props> props, NativeWindow shared)
    {
        return create_scope<glfwWindow>
            (std::move(name), std::move(props), shared);
    }
}
#endif
