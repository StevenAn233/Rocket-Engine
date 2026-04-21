module;
module Context;

import HeapManager;

#ifdef RKE_DEPENDENCY_GLFW
import :glfw;

namespace rke
{
    Scope<Context> Context::create(NativeWindow handle)
        { return create_scope<glfwContext>(handle); }
}
#endif
