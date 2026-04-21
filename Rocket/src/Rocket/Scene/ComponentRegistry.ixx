module;

#include <tuple>
#include <concepts>
#include "rke_macros.h"

export module ComponentRegistry;

import Types;
import Components;
import String;

namespace {
    template<typename Func, typename Tuple>
    struct invocable_with_all { static_assert(false); };

    template<typename Func, typename... TypeIDs>
    struct invocable_with_all<Func, std::tuple<TypeIDs...>>
    { static constexpr bool value{(std::invocable<Func, TypeIDs> && ...)}; };
    // (... && ...) ->
    // (std::invocable<Func, A> && std::invocable<Func, B> && ...)
}

export namespace rke
{
    template<Size N>
    struct FixedString
    {
        char8 buf[N]{};
        constexpr FixedString(const char8 (&s)[N])
            { for(Size i{}; i < N; ++i) buf[i] = s[i]; }
        constexpr operator StringView() const
            { return StringView(buf, N - 1ui64); }
    };

    template<typename T, FixedString Name>
    struct TypeID
    {
        using Type = T;
    private:
        static constexpr FixedString fixed{ Name }; // need to store the buffer
    public:
        static constexpr StringView name{ fixed };
    };

    using ComponentTypes = std::tuple
    <
        TypeID<UUIDComponent         , u8"UUID"           >,
        TypeID<TagComponent          , u8"Tag"            >,
        TypeID<TransformComponent    , u8"Transform"      >,
        TypeID<SpriteComponent       , u8"Sprite"         >,
        TypeID<CameraComponent       , u8"Camera"         >,
        TypeID<Rigidbody2DComponent  , u8"Rigidbody 2D"   >,
        TypeID<BoxCollider2DComponent, u8"Box Collider 2D">,
        TypeID<NativeScriptComponent , u8"Native Script"  >
    >; // for traversing

    struct RKE_API ComponentRegistry
    {
        template<typename Func>
        requires invocable_with_all<Func, ComponentTypes>::value
        static void each(Func&& func)
        {
            std::apply([&](auto... type_ids) {
                (std::invoke(std::forward<Func>(func), type_ids), ...);
            }, ComponentTypes{});
            // (..., ...) ->
            // func(TypeID<A>), func(TypeID<B>), ...
        }
    };
}
