export module EntityAccess;

import Types;

export namespace rke
{
    enum class EntityHandle : uint32 {};
    constexpr EntityHandle entity_handle_null{ 0xFFFFFFFFu };

    struct Contact
    {
        EntityHandle entity_a{ entity_handle_null };
        EntityHandle entity_b{ entity_handle_null };
    };
}
