#pragma once

#define EVENT_CLASS_TYPE(type) \
    static EventType get_static_type() { return EventType::type; } \
    EventType get_event_type() const override { return get_static_type(); } \
    ::rke::StringView get_name() const override \
        { using namespace ::rke::literals; return u8## #type ##_sv; }

#define EVENT_CLASS_CATEGORY(category) int get_category_flags() const override { return category; }
