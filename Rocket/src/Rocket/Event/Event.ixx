// Event system if for one-key-triggered operation(e.g. esc to open menu)
// Input system is for continuous operation(e.g. moving)
module;

#include "rke_macros.h"

export module Event;

import Types;
import String;
import MathUtils;

export namespace rke
{
    enum EventCategory : uint32
    {							   /* flags */
        EventCategoryNull		 = 0,

        EventCategoryApplication = math::bit(0),
        EventCategoryInput		 = math::bit(1),
        EventCategoryKeyboard	 = math::bit(2),
        EventCategoryMouse		 = math::bit(3),
        EventCategoryMouseButton = math::bit(4),

        EventCategoryProject     = math::bit(5),
        EventCategoryScene       = math::bit(6),
        EventCategoryEditor      = math::bit(7),
        EventCategoryClient      = math::bit(8)
    };

    class RKE_API Event
    {
    public:
        friend class EventDispatcher;

        static consteval uint64 gen_type_id(const char8* c_str)
        {
            uint64 hash{ 0xcbf29ce484222325ull };
            for(Size i{}; c_str[i]; i++) {
                hash ^= static_cast<uint64>(c_str[i]);
                hash *= 0x100000001b3ull;
            }
            return hash;
        }

        virtual ~Event() = default;

        virtual StringView get_name() const = 0;
        virtual uint32 get_category_flags() const = 0;
        virtual uint64 get_type_id() const = 0;
        virtual String to_string() const { return get_name(); }

        inline bool belongs_to(EventCategory category) const
            { return !!(get_category_flags() & category); }
        inline StringView get_window_name() const { return window_name_; }
        inline bool handled() const { return handled_; }
    protected:
        Event(StringView window_name) : window_name_(window_name) {} // TO MODIFY
    protected:
        bool handled_{ false }; // For EventDispatcher
        StringView window_name_;
    };
}
