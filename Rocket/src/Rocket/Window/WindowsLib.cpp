module;
module WindowsLib;

import Log;
import HeapManager;
import EventDispatcher;
import ApplicationEvent;

namespace rke
{
    void WindowsLib::on_event(Event& e)
    {
        EventDispatcher(e).dispatch<WindowClosedEvent>
        ([this](WindowClosedEvent& e)
        {
            CORE_INFO(e);
            remove(e.get_window_name());
            return true;
        });
        if(e.handled()) return;
        for(auto& [_, window] : map_)
            window->on_event(e);
    }

    void WindowsLib::update_all(float dt)
    {
        for(auto& [_, window] : map_)
            window->on_update(dt);
    }

    void WindowsLib::render_all()
    {
        for(auto& [_, window] : map_)
            window->on_render();
    }

    Window& WindowsLib::load(String name, Scope<Window::Props> props)
    {
        CORE_ASSERT(main_window_, u8"WindowsLib: Main window empty!");
        Scope<Window> window{ Window::create(std::move(name),
            std::move(props), main_window_->get_context()) };
        return add(std::move(window));
    }

    Window& WindowsLib::load_main(Scope<Window::Props> props)
    {
        Scope<Window> window{ Window::create
            (u8"main", std::move(props), NativeWindow()) };
        main_window_ = window.get();
        return add(std::move(window));
    }

    void WindowsLib::remove(const String& name)
        { if(exists(name)) map_[name]->should_close(true); }

    void WindowsLib::remove_main() { remove(u8"main"); }

    Window& WindowsLib::operator[](const String& name)
    {
        CORE_ASSERT(exists(name), u8"WindowsLib: Window '{}' not found!", name);
        return *(map_.at(name).get());
    }

    const Window& WindowsLib::operator[](const String& name) const
    {
        CORE_ASSERT(exists(name), u8"WindowsLib: Window '{}' not found!", name);
        return *(map_.at(name).get());
    }

    Window& WindowsLib::add(Scope<Window> window)
    {
        if(load_callback_) {
            WindowsLib::make_context_current(window->get_context());
            load_callback_(*(window.get()));
        }
        const String& name{ window->get_name() };
        CORE_ASSERT(!exists(name), u8"WindowsLib: Name already exists!");
        map_.emplace(name, std::move(window));
        return (*this)[name];
    }
}
