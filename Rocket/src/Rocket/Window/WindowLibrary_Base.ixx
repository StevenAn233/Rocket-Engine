module;

#include <unordered_map>
#include <memory>

export module WindowLibrary:Base;

import Types;
import Window;
import HeapManager;
import String;

export namespace rke
{
    class WindowLibrary
    {
    public:
        friend struct std::default_delete<WindowLibrary>;

        using WindowsMap = std::unordered_map<String, Scope<Window>>;

        WindowLibrary(const WindowLibrary&) = delete;
        WindowLibrary& operator=(const WindowLibrary&) = delete;
        WindowLibrary(WindowLibrary&&) = delete;
        WindowLibrary& operator=(WindowLibrary&&) = delete;

        virtual void refresh() = 0;

        virtual void load(const Window::WindowProps& props) = 0;
        virtual Window* operator[](const String& name) = 0;
        virtual const Window* operator[](const String& name) const = 0;

        virtual void remove(const String& name) = 0;
        virtual bool is_empty() const = 0;

        virtual WindowsMap& get_mut() = 0;
        virtual const WindowsMap& get() const = 0;

        virtual void make_master_context_current () = 0;
        virtual NativeWindow get_current_context()	const = 0;
        virtual NativeWindow get_master_context () const = 0;

        virtual void add(Scope<Window> window) = 0;
        virtual bool exists(const String& name) const = 0;

        virtual Size size() const = 0;

        static Scope<WindowLibrary> create();
    protected:
        WindowLibrary() = default;
        virtual ~WindowLibrary() = default;
    };
}
