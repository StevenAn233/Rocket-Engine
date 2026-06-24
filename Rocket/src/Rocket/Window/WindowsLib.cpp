module;
module WindowsLib;

import HeapManager;
import Window;
import Log;

namespace rke
{
    Window* WindowsLib::add(Scope<Window> window)
    {
        CORE_ASSERT(!exists(window->get_name()), u8"WindowsLib: Name already exists!");
        Window* ret{ window.get() };
        windows_[window->get_name()] = std::move(window);
        return ret;
    }

    Window* WindowsLib::operator[](const String& name)
    {
        CORE_ASSERT(exists(name), u8"WindowsLib: Window '{}' not found!", name);
        return windows_.at(name).get(); // not const
    }

    const Window* WindowsLib::operator[](const String& name) const
    {
        CORE_ASSERT(exists(name), u8"WindowsLib: Window '{}' not found!", name);
        return windows_.at(name).get(); // const
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
