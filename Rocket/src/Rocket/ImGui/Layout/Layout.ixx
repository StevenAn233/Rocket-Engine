module;

#include <bit>
#include <concepts>
#include <functional>
#include <optional>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Layout;

import String;
import Types;

namespace rke
{
    static consteval uint64 gen_id(const char8* c_str)
    {
        uint64 hash{ 0xcbf29ce484222325ull };
        for(Size i{}; c_str[i]; i++) {
            hash ^= static_cast<uint64>(c_str[i]);
            hash *= 0x100000001b3ull;
        }
        return hash;
    }
}

export namespace rke::layout
{
    RKE_API bool begin_table_impl(StringView text);
    RKE_API void end_table_impl();

    template<StringLiteral Str, typename Callback>
    requires std::invocable<Callback>
    inline void two_columns_table(Callback&& callback)
    {
        if(begin_table_impl(StringView(Str.data)))
        {
            std::invoke(std::forward<Callback>(callback));
            end_table_impl();
        }
    }

    RKE_API bool begin_tree_node_branch_impl(StringView name, uint32 flags, void* id);
    RKE_API void end_tree_node_branch_impl();

    template<StringLiteral Str, typename Callback>
    requires std::invocable<Callback>
    inline void tree_node_branch(Callback&& callback,
        uint32 extra_flags = 0, void* id = nullptr)
    {
        if(begin_tree_node_branch_impl(StringView(Str.data), extra_flags,
            id ? id : std::bit_cast<void*>(gen_id(Str.data))))
        {
            std::invoke(std::forward<Callback>(callback));
            end_tree_node_branch_impl();
        }
    }

    RKE_API bool begin_tree_node_leaf_impl(StringView name, void* id);
    RKE_API void end_tree_node_leaf_impl();

    template<StringLiteral Str, typename Callback>
    requires std::invocable<Callback>
    inline void tree_node_leaf(Callback&& callback, void* id = nullptr)
    {
        if(begin_tree_node_leaf_impl(StringView(Str.data),
            id ? id : std::bit_cast<void*>(gen_id(Str.data))))
        {
            std::invoke(std::forward<Callback>(callback));
            end_tree_node_leaf_impl();
        }
    }

    RKE_API bool drag_float_control_impl(float& value,
        float pan_speed, float reset_value,
        std::optional<glm::vec2> range, StringView format);

    template<StringLiteral Str>
    inline bool drag_float_control(float& value,
        float pan_speed = 0.1f, float reset_value = 0.0f,
        std::optional<glm::vec2> range = glm::vec2(0.0f, 0.0f),
        StringView format = u8"%.2f")
    {
        bool data_changed{ false };
        two_columns_table<Str>([&]()
        {
            data_changed = drag_float_control_impl(value,
                pan_speed, reset_value, range, format);
        });
        return data_changed;
    }

    RKE_API bool drag_float2_control_impl(glm::vec2& value,
        float pan_speed, glm::vec2 reset_value,
        std::optional<glm::vec2> x_range,
        std::optional<glm::vec2> y_range,
        StringView format);

    template<StringLiteral Str>
    inline bool drag_float2_control(glm::vec2& value,
        float pan_speed = 0.1f, glm::vec2 reset_value = glm::vec2(0.0f),
        std::optional<glm::vec2> x_range = glm::vec2(0.0f, 0.0f),
        std::optional<glm::vec2> y_range = glm::vec2(0.0f, 0.0f),
        StringView format = u8"%.2f")
    {
        bool data_changed{ false };
        two_columns_table<Str>([&]()
        {
            data_changed = drag_float2_control_impl(value,
                pan_speed, reset_value, x_range, y_range, format);
        });
        return data_changed;
    }

    RKE_API bool drag_float3_control_impl(glm::vec3& values,
        float pan_speed, glm::vec3 reset_value,
        std::optional<glm::vec2> x_range,
        std::optional<glm::vec2> y_range,
        std::optional<glm::vec2> z_range,
        StringView format);

    template<StringLiteral Str>
    inline bool drag_float3_control(glm::vec3& value,
        float pan_speed = 0.1f, glm::vec3 reset_value = glm::vec2(0.0f),
        std::optional<glm::vec2> x_range = glm::vec2(0.0f, 0.0f),
        std::optional<glm::vec2> y_range = glm::vec2(0.0f, 0.0f),
        std::optional<glm::vec2> z_range = glm::vec2(0.0f, 0.0f),
        StringView format = u8"%.2f")
    {
        bool data_changed{ false };
        two_columns_table<Str>([&]()
        {
            data_changed = drag_float3_control_impl(value,
                pan_speed, reset_value, x_range, y_range, z_range, format);
        });
        return data_changed;
    }
}
