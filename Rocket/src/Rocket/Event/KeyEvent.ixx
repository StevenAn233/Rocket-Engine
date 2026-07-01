module;

#include "rke_macros.h"
#include "Events/event_macros.h"

export module KeyEvent;

import Event;
import Types;
import Keys;

export namespace rke
{
    class RKE_API CharTypedEvent : public Event
    {
    public:
        CharTypedEvent(StringView name, uint32 code_point)
            : Event(name), code_point_(static_cast<uint8>(code_point)) {}

        String to_string() const override
        {
            return String::format(u8"{}: {}", get_name(),
                static_cast<char>(code_point_));
        }

        EVENT_CLASS_TYPE(CharTyped);
        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);
    private:
        uint8 code_point_;
    };

    class RKE_API KeyEvent : public Event
    {
    public:
        Key get_key() const { return key_; }
        
        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);
    protected:
        KeyEvent(StringView name, Key key) : Event(name), key_(key) {}
    protected:
        Key key_;
    };

    class RKE_API KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(StringView name, Key key, bool held = false)
            : KeyEvent(name, key), is_held_(held) {}

        bool is_held() const { return is_held_; }
        String to_string() const override
        {
            return String::format(u8"{}: {} [held: {}]",
                get_name(), key_to_string(key_), is_held_);
        }

        EVENT_CLASS_TYPE(KeyPressed);
    private:
        bool is_held_;
    };

    class RKE_API KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(StringView name, Key key) : KeyEvent(name, key) {}

        String to_string() const override
        {
            return String::format(u8"{}: {}",
                get_name(), key_to_string(key_));
        }

        EVENT_CLASS_TYPE(KeyReleased);
    };
}
