module;
module WindowsLib;

import Log;
import Renderer;
import HeapManager;
import EventDispatcher;
import ApplicationEvent;

namespace rke
{
    void WindowsLib::loop()
    {
        if(!valid()) return; // invalidating happens below
        for(auto& [_, window] : map_)
        {
            if(window->should_close())
            {
                CORE_WARN(u8"WindowsLib: Window '{}' "
                    u8"should have been closed!", window->get_name());
                continue;
            }
            window->make_context_current();
            window->on_update();
            window->on_render();
        }
        refresh();
    }

    void WindowsLib::on_event(Event& e)
    {
        EventDispatcher(e).dispatch<WindowClosedEvent>
        ([this](WindowClosedEvent& e)
        {
            CORE_INFO(e);
            make_useless(e.get_window_name());
            return true;
        });
        if(e.handled()) return;

        for(auto& [name, window] : map_)
        {
            if(e.get_window_name() == name)
            {
                window->on_event(e);
                return;
            }
        }
    }

    Window& WindowsLib::load(String name, Scope<Window::Props> props)
    {
        CORE_ASSERT(main_window_, u8"WindowsLib: Main window empty!");
        Scope<Window> window{ Window::create(std::move(name),
            std::move(props), main_window_->get_context()) };
        return add(std::move(window));
    }

    void WindowsLib::make_useless(const String& name)
        { if(exists(name)) map_[name]->should_close(true); }

    Window& WindowsLib::load_main(Scope<Window::Props> props)
    {
        Scope<Window> window{ Window::create
            (u8"main", std::move(props), NativeWindow()) };
        Renderer::init();
        main_window_ = window.get();
        return add(std::move(window));
    }

    // do not set main_window_ to nullptr here
    void WindowsLib::make_main_useless() { make_useless(u8"main"); }

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

    Window& WindowsLib::get_main()
    {
        CORE_ASSERT(main_window_, u8"WindowsLib: Main window null!");
        return *main_window_;
    }

    const Window & WindowsLib::get_main() const
    {
        CORE_ASSERT(main_window_, u8"WindowsLib: Main window null!");
        return *main_window_;
    }

    Window& WindowsLib::add(Scope<Window> window)
    {
        const String& name{ window->get_name() };
        CORE_ASSERT(!exists(name), u8"WindowsLib: Name already exists!");
        map_.emplace(name, std::move(window));
        return (*this)[name];
    }
}
