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
import ApplicationEvent;

export namespace rke
{
    class RKE_API WindowsLib
    {
    public:
        friend class Application;

        using WindowsMap = std::unordered_map<String, Scope<Window>>;

        WindowsLib(const WindowsLib&) = delete;
        WindowsLib& operator=(const WindowsLib&) = delete;
        WindowsLib(WindowsLib&&) = delete;
        WindowsLib& operator=(WindowsLib&&) = delete;

        static NativeWindow get_current_context();
        static void make_context_current(NativeWindow context);

        void refresh();

        Window& load(String name, Scope<Window::Props> props);
        Window& load_main(Scope<Window::Props> props);

        void remove(const String& name);
        void remove_main();

        Window& operator[](const String& name);
        const Window& operator[](const String& name) const;
        inline Window& get_main() { return *main_window_; }
        inline const Window& get_main() const { return *main_window_; }

        inline Size size () const { return map_.size (); }
        inline bool empty() const { return map_.empty(); }
        inline bool exists(const String& name) const { return map_.contains(name); }
    private:
        WindowsLib(std::function<void(Window&)> callback);
        ~WindowsLib();

        void on_event(Event& e);
        bool on_window_closed(rke::WindowClosedEvent& e);
        
        void update_all(float dt);
        void render_all();
        Window& add(Scope<Window> window);
    private:
        WindowsMap map_{};
        Window* main_window_{};
        std::function<void(Window&)> load_callback_;
    };
}
