module;

#include "rke_macros.h"
#include "Events/event_macros.h"

export module Event:Application;

import :Base;
import Types;
import String;

export namespace rke
{
    class RKE_API WindowEvent : public Event
    {
    public:
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    protected:
        WindowEvent(StringView title) : Event(title) {}
    private:
        String window_title_;
    };

    class RKE_API WindowResizedEvent : public WindowEvent
    {
    public:
        WindowResizedEvent(StringView title, uint32 width, uint32 height)
            : WindowEvent(title), width_(width), height_(height) {}

        uint32 get_width () const { return width_;  }
        uint32 get_height() const { return height_; }

        String to_string() const override
            { return String::format(u8"{}: to({}, {})", get_name(), width_, height_); }

        EVENT_CLASS_TYPE(WindowResized)
    private:
        uint32 width_, height_;
    };

    using ViewportResizedEvent = WindowResizedEvent;

    class RKE_API WindowClosedEvent : public WindowEvent
    {
    public:
        WindowClosedEvent(StringView title) : WindowEvent(title) {}

        String to_string() const override { return get_name(); }

        EVENT_CLASS_TYPE(WindowClosed)
    };

    class RKE_API AppTickedEvent : public Event
    {
    public:
        AppTickedEvent(StringView title) : Event(title) {}

        EVENT_CLASS_TYPE(AppTicked)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class RKE_API AppUpdatedEvent : public Event
    {
    public:
        AppUpdatedEvent(StringView title) : Event(title) {}

        EVENT_CLASS_TYPE(AppUpdated)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class RKE_API AppRenderedEvent : public Event
    {
    public:
        AppRenderedEvent(StringView title) : Event(title) {}

        EVENT_CLASS_TYPE(AppRendered)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class RKE_API NewSceneEvent : public Event
    {
    public:
        NewSceneEvent(StringView title) : Event(title) {}

        EVENT_CLASS_TYPE(NewScene)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
}
