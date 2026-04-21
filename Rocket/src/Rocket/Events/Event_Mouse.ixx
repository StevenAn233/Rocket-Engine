module;

#include "rke_macros.h"
#include "Events/event_macros.h"

export module Event:Mouse;

import :Base;
import MouseButtons;

export namespace rke
{
    class RKE_API MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(StringView title, float x, float y)
            : Event(title), x_coord_(x), y_coord_(y) {}

        float get_x() const { return x_coord_; }
        float get_y() const { return y_coord_; }

        String to_string() const override
            { return String::format(u8"{}: at({}, {})", get_name(), x_coord_, y_coord_); }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    private:
        float x_coord_, y_coord_;
    };

    class RKE_API MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(StringView title, float x_offset, float y_offset)
            : Event(title), x_offset_(x_offset), y_offset_(y_offset) {}

        float get_x_offset() const { return x_offset_; }
        float get_y_offset() const { return y_offset_; }

        String to_string() const override
            { return String::format(u8"{}: offset({}, {})", get_name(), x_offset_, y_offset_); }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    private:
        float x_offset_, y_offset_;
    };

    class RKE_API MouseButtonEvent : public Event /* Abstract(Category Level) */
    {
    public:
        Mouse get_mouse_button() const { return button_; }

        EVENT_CLASS_CATEGORY(EventCategoryInput |
                             EventCategoryMouse |
                             EventCategoryMouseButton)
    protected:
        MouseButtonEvent(StringView title, Mouse button)
            : Event(title), button_(button) {}
    protected:
        Mouse button_;
    };

    class RKE_API MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(StringView title, Mouse button)
            : MouseButtonEvent(title, button) {}

        String to_string() const override {
            return String::format(u8"{}: {}",
                get_name(), mouse_button_to_string(button_));
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class RKE_API MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(StringView title, Mouse button)
            : MouseButtonEvent(title, button) {}

        String to_string() const override {
            return String::format(u8"{}: {}",
                get_name(), mouse_button_to_string(button_));
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };
}
