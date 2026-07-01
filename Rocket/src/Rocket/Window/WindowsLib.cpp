module;
module WindowsLib;

import Log;
import HeapManager;
import Window;
import Renderer2D;

namespace rke
{
    void WindowsLib::on_event(Event& e)
    {
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
        {
            Renderer2D::reset_stats();
            window->on_render();
            window->on_imgui_render();
        }
    }

    Window& WindowsLib::add(Scope<Window> window)
    {
        const String& name{ window->get_name() };
        CORE_ASSERT(!exists(name), u8"WindowsLib: Name already exists!");
        map_.emplace(name, std::move(window));
        return (*this)[name];
    }

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
}

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke
{
    Scope<WindowsLib> WindowsLib::create()
        { return create_scope<glfwWindowsLib>(); }
}
#endif
