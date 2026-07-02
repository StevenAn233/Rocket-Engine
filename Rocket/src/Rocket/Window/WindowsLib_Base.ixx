module;

#include <unordered_map>
#include <memory>

export module WindowsLib:Base;

import Types;
import Window;
import HeapManager;
import String;
import Event;

export namespace rke
{
    class WindowsLib
    {
    public:
        friend class Application;
        friend struct std::default_delete<WindowsLib>;

        using WindowsMap = std::unordered_map<String, Scope<Window>>;

        WindowsLib(const WindowsLib&) = delete;
        WindowsLib& operator=(const WindowsLib&) = delete;
        WindowsLib(WindowsLib&&) = delete;
        WindowsLib& operator=(WindowsLib&&) = delete;

        virtual void refresh() = 0;

    // TO REMOVE
        virtual NativeWindow get_current_context() const = 0;
        virtual NativeWindow get_master_context () const = 0;
        virtual void make_master_context_current() = 0;

        static Scope<WindowsLib> create();
    protected:
        Window& add(Scope<Window> window);
        virtual Window& load(String name, Scope<Window::Props> props) = 0;
        // virtual Window& create_main() = 0;
    private:
        void on_event(Event& e);
        void update_all(float dt);
        void render_all();
        
        inline Size size () const { return map_.size (); }
        inline bool empty() const { return map_.empty(); }
        inline bool exists(const String& name) const { return map_.contains(name); }
        inline void remove(const String& name)
            { if(exists(name)) map_[name]->should_close(true); }

    // getters
        Window& operator[](const String& name);
        const Window& operator[](const String& name) const;
        inline Window& get_main() { return operator[](u8"main"); }
        inline const Window& get_main() const { return operator[](u8"main"); }
    protected:
        WindowsLib() = default;
        virtual ~WindowsLib() = default;
    protected:
        WindowsMap map_{};
        NativeWindow main_context_{};
    };
}
