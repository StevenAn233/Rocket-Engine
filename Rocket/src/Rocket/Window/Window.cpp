module;
module Window;

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke {
    Scope<Window> Window::create(Scope<Props> props, NativeWindow handle)
        { return create_scope<glfwWindow>(std::move(props), handle); }
}
#endif
