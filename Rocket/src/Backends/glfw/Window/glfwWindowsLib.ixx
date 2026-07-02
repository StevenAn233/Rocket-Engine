export module WindowsLib:glfw;

import :Base;
import Types;

namespace rke
{
    class glfwWindowsLib : public WindowsLib
    {
    public:
        glfwWindowsLib();
        ~glfwWindowsLib() override;

        void refresh() override;

    // TO REMOVE
        NativeWindow get_current_context() const override;
        NativeWindow get_master_context () const override;
        void make_master_context_current() override;
    private:
        Window& load(String name, Scope<Window::Props> props) override;
    private:
    // TO REMOVE
        void* master_context_{};
    };
}
