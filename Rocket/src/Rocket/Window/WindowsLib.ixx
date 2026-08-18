module;

#include <memory>
#include <functional>
#include <unordered_map>
#include "rke_macros.h"

export module WindowsLib;

import Types;
import Window;
import NativeWindow;
import HeapManager;
import String;
import Event;

export namespace rke
{
    class RKE_API WindowsLib
    {
    public:
        using WindowsMap = std::unordered_map<String, Scope<Window>>;

        WindowsLib(std::function<void(Window&)> callback);
        ~WindowsLib();

        WindowsLib(const WindowsLib&) = delete;
        WindowsLib& operator=(const WindowsLib&) = delete;
        WindowsLib(WindowsLib&&) = delete;
        WindowsLib& operator=(WindowsLib&&) = delete;

        static NativeWindow get_current_context();
        static void make_context_current(NativeWindow context);
        static bool is_context_current(NativeWindow context);

        void loop();
        void on_event(Event& e);

        Window& load_main(Scope<Window::Props> props);
        void remove_main();

        Window& load(String name, Scope<Window::Props> props);
        void remove(const String& name);
        
        Window& operator[](const String& name);
        const Window& operator[](const String& name) const;
        Window& get_main() { return *main_window_; }
        const Window& get_main() const { return *main_window_; }

        inline Size size () const { return map_.size (); }
        inline bool empty() const { return map_.empty(); }
        inline bool exists(const String& name) const { return map_.contains(name); }
        inline bool valid() const { return main_window_ && !main_window_->should_close(); }
    private:
        void refresh();
        Window& add(Scope<Window> window);
    private:
        WindowsMap map_{};
        Window* main_window_{};
        std::function<void(Window&)> load_callback_;
    };
}
