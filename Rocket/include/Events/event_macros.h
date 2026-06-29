#pragma once

#define EVENT_CLASS_TYPE(__type) \
    static uint64_t type_id() { return ::rke::Event::gen_type_id(u8## #__type); } \
    uint64_t get_type_id() const override { return type_id(); } \
    ::rke::StringView get_name() const override \
        { using namespace ::rke::literals; return u8## #__type ##_sv; }

#define EVENT_CLASS_CATEGORY(__category) \
    uint32_t get_category_flags() const override { return __category; }
