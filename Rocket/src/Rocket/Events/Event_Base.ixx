module;

#include "rke_macros.h"

export module Event:Base;

import Types;
import String;

export namespace rke
{
    enum class EventType
    {
        TypeNull = 0,

        // Application Event
        WindowClosed, WindowResized,
        WindowFocusedLostFocusMoved,
        AppTicked, AppUpdated, AppRendered,
        NewScene,

        // Key Event
        KeyPressed, KeyReleased, CharTyped,

        // Mouse Event
        MouseButtonPressed, MouseButtonReleased,
        MouseMoved, MouseScrolled
    };

    consteval uint32 bit(int x) { return (1U << x); }

    enum EventCategory
    {							/* flags */
        EventCategoryNull		 = 0,
        EventCategoryApplication = bit(0),
        EventCategoryInput		 = bit(1),
        EventCategoryKeyboard	 = bit(2),
        EventCategoryMouse		 = bit(3),
        EventCategoryMouseButton = bit(4)
    };

    class RKE_API Event /* Abstract(Base) */
    {
    public:
        friend class EventDispatcher;
        friend class ImGuiLayer;

        virtual ~Event() = default;

        virtual EventType  get_event_type()	    const = 0;
        virtual StringView get_name()		    const = 0;
        virtual int		   get_category_flags() const = 0;

        virtual String to_string() const { return get_name(); }
        // when dynamically binding(reference or pointer of base class),
        // virtual function won't be inline(though it is called to be inline)

        StringView get_window_title() const { return window_title_; }
        bool is_in_category(EventCategory category) const
        {
            return get_category_flags() & category;
            // bit-with: see if categorys are the same
            // (if event is in the category)
        }
        bool handled() const { return handled_; } // For Application::on_event()
    protected:
        Event(StringView window_title) : window_title_(window_title) {}
    protected:
        bool handled_{ false }; // For Dispatcher Using
        StringView window_title_;
    };
}
