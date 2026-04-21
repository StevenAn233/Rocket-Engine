module;

#include <glfw/glfw3.h>
#include "rke_macros.h"

export module WindowLibrary:glfw;

import :Base;
import Types;

namespace rke
{
    class glfwWindowLibrary : public WindowLibrary
    {
    public:
        glfwWindowLibrary();
        ~glfwWindowLibrary() override;

        void refresh() override;

        void load(const Window::WindowProps& props) override;
        Window* operator[](const String& name) override;
        const Window* operator[](const String& name) const override;

        void remove(const String& name) override
        {
            if(exists(name))
                windows_[name]->should_close(true);
            // actually removed in refresh()
        }
        bool is_empty() const override { return windows_.empty(); }

        WindowsMap& get_mut() override { return windows_; }
        const WindowsMap& get() const override { return windows_; }

        void make_master_context_current() override;
        NativeWindow get_current_context() const override
            { return NativeWindow(glfwGetCurrentContext()); }
        NativeWindow get_master_context () const override
            { return NativeWindow(master_context_); }

        Size size() const override { return windows_.size(); }

        void add(Scope<Window> window) override;
        bool exists(const String& name) const override
            { return windows_.find(name) != windows_.end(); }
    private:
        WindowsMap windows_{};
        GLFWwindow* master_context_{};
    };
}
