module;

#include "rke_macros.h"
#include "Events/event_macros.h"

export module ApplicationEvent;

import Event;
import Types;
import String;

export namespace rke
{
    class RKE_API WindowEvent : public Event
    {
    public:
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    protected:
        WindowEvent(StringView name) : Event(name) {}
    };

    class RKE_API WindowResizedEvent : public WindowEvent
    {
    public:
        WindowResizedEvent(StringView name, uint32 width, uint32 height)
            : WindowEvent(name), width_(width), height_(height) {}

        uint32 get_width () const { return width_;  }
        uint32 get_height() const { return height_; }

        String to_string() const override
        {
            return String::format(u8"{}: to({}, {})",
                get_name(), width_, height_);
        }

        EVENT_CLASS_TYPE(WindowResized);
    private:
        uint32 width_, height_;
    };

    class RKE_API WindowClosedEvent : public WindowEvent
    {
    public:
        WindowClosedEvent(StringView name) : WindowEvent(name) {}

        String to_string() const override { return get_name(); }

        EVENT_CLASS_TYPE(WindowClosed);
    };

    class RKE_API AppTickedEvent : public Event
    {
    public:
        AppTickedEvent(StringView name) : Event(name) {}

        EVENT_CLASS_TYPE(AppTicked);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    };

    class RKE_API AppUpdatedEvent : public Event
    {
    public:
        AppUpdatedEvent(StringView name) : Event(name) {}

        EVENT_CLASS_TYPE(AppUpdated);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    };

    class RKE_API AppRenderedEvent : public Event
    {
    public:
        AppRenderedEvent(StringView name) : Event(name) {}

        EVENT_CLASS_TYPE(AppRendered);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    };
}
