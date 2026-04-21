module;
module Window;

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke {
    Scope<Window> Window::create(const WindowProps& props, NativeWindow handle)
        { return create_scope<glfwWindow>(props, handle); }
}
#endif
