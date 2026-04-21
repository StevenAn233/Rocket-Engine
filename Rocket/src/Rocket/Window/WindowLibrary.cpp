module;
module WindowLibrary;

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke {
    Scope<WindowLibrary> WindowLibrary::create()
        { return create_scope<glfwWindowLibrary>(); }
}
#endif
