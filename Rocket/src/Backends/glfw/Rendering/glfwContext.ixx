module;

#include <GLFW/glfw3.h>

export module Context:glfw;

import :Base;

namespace rke
{
    class glfwContext : public Context
    {
    public:
        glfwContext(NativeWindow handle);

        void init() override;
        void swap_buffers() override;
    private:
        GLFWwindow* handle_;
    };
}
