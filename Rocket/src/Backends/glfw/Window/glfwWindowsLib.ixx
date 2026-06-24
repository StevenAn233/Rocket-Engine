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
        Window* load(Window::WindowProps props) override;
        NativeWindow get_current_context() const override;
        NativeWindow get_master_context () const override;
        void make_master_context_current() override;
    private:
        void* master_context_{};
    };
}
