module;

#include "rke_macros.h"
#include "Events/event_macros.h"

export module Event:Key;

import :Base;
import Types;
import Keys;

export namespace rke
{
    class RKE_API CharTypedEvent : public Event
    {
    public:
        CharTypedEvent(StringView title, uint32 code_point)
            : Event(title), code_point_(code_point) {}

        String to_string() const override
        {
            return String::format(u8"{}: {}",
                get_name(), static_cast<char>(code_point_));
        }

        EVENT_CLASS_TYPE(CharTyped)
        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
    private:
        uint8 code_point_;
    };

    class RKE_API KeyEvent : public Event
    {
    public:
        Key get_key() const { return key_; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
    protected:
        KeyEvent(StringView title, Key key) : Event(title), key_(key) {}
    protected:
        Key key_;
    };

    class RKE_API KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(StringView title, Key key, bool held = false)
            : KeyEvent(title, key), is_held_(held) {}

        bool is_held() const { return is_held_; }
        String to_string() const override
        {
            return String::format(u8"{}: {} [held: {}]",
                get_name(), key_to_string(key_), is_held_);
        }

        EVENT_CLASS_TYPE(KeyPressed)
    private:
        bool is_held_;
    };

    class RKE_API KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(StringView title, Key key) : KeyEvent(title, key) {}

        String to_string() const override
            { return String::format(u8"{}: {}", get_name(), key_to_string(key_)); }

        EVENT_CLASS_TYPE(KeyReleased)
    };
}
