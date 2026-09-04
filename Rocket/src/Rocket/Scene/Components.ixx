module;

#include <tuple>
#include <utility>
#include <concepts>
#include <cstring>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Components;

import Types;
import String;
import SceneCamera;
import PhysicsLayers;
import AssetsManager;
import AssetAccess;
import ScriptAccess;
import Mesh;
import UUID;
import GTexture;

// put datas that you wanna deal with simultaneously in the same component
export namespace rke
{
    struct RKE_API IdentityComponent // MUST OWNED
    {
        static constexpr Size tag_size{ 64 };

        char8 tag[tag_size];
        UUID uuid;

        IdentityComponent()
            : tag({}), uuid() { std::memcpy(&tag[0], u8"Null", 4); }
        IdentityComponent(const char8* str)
            : tag({}), uuid() { std::memcpy(&tag[0], str, tag_size - 1); }
        IdentityComponent(const char8* str, UUID specified)
            : tag({}), uuid(specified) { std::memcpy(&tag[0], str, tag_size - 1); }
        IdentityComponent(const IdentityComponent&) = default;
    };

    struct RKE_API TransformComponent // MUST OWNED
    {
        glm::vec3 translation{ 0.0f };
        glm::vec3 rotation{ 0.0f };
        glm::vec3 scale{ 1.0f };
        bool locked{ false };

        TransformComponent() = default;
        TransformComponent(glm::vec3 tra, glm::vec3 rot, glm::vec3 scl)
            : translation(std::move(tra)), rotation(std::move(rot)), scale(std::move(scl)) {}
        TransformComponent(const TransformComponent&) = default;

        glm::mat4 get_transform() const;
    };

    struct RKE_API CameraComponent
    {
        SceneCamera camera{};
        bool aspect_ratio_fixed{ false };

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
    };

    enum class BlendingMode : uint32
    {
        Opaque = 0,
        Transparent
    };

    struct RKE_API SpriteComponent
    {
        glm::vec4 color{ 1.0f }; // may modify
        BlendingMode blending_mode{ BlendingMode::Opaque };
        int rendering_layer{ 0 };

    /* -runtime cache(do not serialize): copiable- */
        const Mesh* quad; // may modify

        glm::vec2 uv_scale { 1.0f }; // Update: SceneRenderer
        glm::vec2 uv_offset{ 0.0f }; // Update: SceneRenderer
        
        SpriteComponent();
        SpriteComponent(const SpriteComponent&) = default;
    };

    namespace sprite
    {
        inline glm::vec2 compute_uv_scale
        (std::pair<int, int> cell_size, uint32 tex_w, uint32 tex_h)
        {
            return glm::vec2 (
                float(cell_size.first ) / float(tex_w),
                float(cell_size.second) / float(tex_h)
            );
        }

        inline glm::vec2 compute_uv_offset
        (std::pair<int, int> cell_coords, glm::vec2 uv_scale)
        {
            return glm::vec2 (
                float(cell_coords.first ) * uv_scale.x,
                float(cell_coords.second) * uv_scale.y
            );
        }
    }

// Requires to own a SpriteComponent:
    struct RKE_API TextureComponent
    {
        AssetUUID tex_uuid{ 0 };

        GTextureSettings gtex_settings{};
        std::pair<int, int> cell_size  { 1, 1 }; // pixel counts
        std::pair<int, int> cell_coords{ 0, 0 };

    /* -runtime cache(do not serialize): copiable- */
        AssetResolve resolved_tex{};

        TextureComponent() = default;
        TextureComponent(const TextureComponent& other) = default;
    };

    struct RKE_API AnimatorComponent
    {
        static constexpr Size clip_name_cap{ 64 };

        AssetUUID anim_uuid{ 0 };
        char8 clip_name[clip_name_cap]{};
        GTextureSettings gtex_settings{};

    /* -runtime cache(do not serialize): copiable- */
        AssetHandle curr_tex_handle{ asset_handle_null }; // Update: AnimatorSystem
        std::pair<int, int> curr_cell_size  { 1, 1 };
        std::pair<int, int> curr_cell_coords{ 0, 0 };

        AnimatorComponent() = default;
        AnimatorComponent(const AnimatorComponent& other) = default;

        inline bool has_clip() const { return clip_name[0] != u8'\0'; }
        inline StringView get_clip_name() const { return StringView(clip_name); }
        void set_clip_name(StringView name); // truncated to buffer
    };

    enum class BodyType : uint32
    {
        Unsimulated = 0,
        Simulated   = 1
    };

    struct RKE_API Rigidbody2DComponent
    {
        BodyType type{ BodyType::Unsimulated };
        bool rotation_fixed{ false };

        float mass{ 0.0f };
        glm::vec2 velocity{ 0.0f };
        float angular_velocity{ 0.0f };

    /* -runtime cache(do not serialize): uncopiable- */
        uint64 body_id{};

        Rigidbody2DComponent() = default;
        Rigidbody2DComponent(const Rigidbody2DComponent& other)
            : type(other.type)
            , rotation_fixed(other.rotation_fixed)
            , mass(other.mass)
            , velocity(other.velocity)
            , angular_velocity(other.angular_velocity) {}
    };

    enum class ColliderType : uint32
    {
        Solid  = 0, // physical collision + contact events
        Sensor = 1, // contact events only, no physical response
        OneWay = 2  // one-way platform: only collides from above
    };

    struct RKE_API BoxCollider2DComponent
    {
        ColliderType type{ ColliderType::Solid };
        uint8 layer_index{ 0 }; // 0 for default

        glm::vec2 offset{ 0.0f, 0.0f };
        glm::vec2 half_extent{ 0.5f, 0.5f }; // half w, half h (0.0 to 1.0)
        
        float density{ 1.0f };
        float friction{ 0.5f };
        float restitution{ 0.0f }; // 'bounciness'

    /* -runtime cache(do not serialize): uncopiable- */
        uint64 shape_id{};
        glm::vec2 resolved_shape_size{};

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent& other)
            : type		 (other.type	   )
            , layer_index(other.layer_index)
            , offset	 (other.offset	   )
            , half_extent(other.half_extent)
            , density	 (other.density	   )
            , friction	 (other.friction   )
            , restitution(other.restitution) {}
    };

    struct RKE_API NativeScriptComponent
    {
        ScriptType script_type{ script_type_null };

    /* -runtime cache(do not serialize): uncopiable- */
        ScriptType resolved_script_type{ script_type_null };
        void* script_handle{}; // will be cleared by ScriptManager

        NativeScriptComponent() = default;
        NativeScriptComponent(const NativeScriptComponent& other)
            : script_type(other.script_type) {}
    };

// Registry
    template<typename Func, typename Tuple>
    struct invocable_with_all { static_assert(false); };

    template<typename Func, typename... TypeIDs>
    struct invocable_with_all<Func, std::tuple<TypeIDs...>>
    {
        static constexpr bool value
        { (std::invocable<Func, TypeIDs> && ...) };
    };
    // (... && ...) ->
    // (std::invocable<Func, A> && std::invocable<Func, B> && ...)

    template<typename T, StringLiteral Name>
    struct TypeID
    {
        using Type = T;
    private:
        static constexpr StringLiteral fixed{ Name }; // need to store the buffer
    public:
        static constexpr StringView name{ fixed.data };
    };

    using ComponentTypes = std::tuple
    <
        TypeID<IdentityComponent , u8"Identity" >,
        TypeID<TransformComponent, u8"Transform">,
        TypeID<CameraComponent   , u8"Camera"   >,
        TypeID<SpriteComponent   , u8"Sprite"   >,
        TypeID<TextureComponent  , u8"Texture"  >,
        TypeID<AnimatorComponent , u8"Animator" >,
        TypeID<Rigidbody2DComponent   , u8"Rigidbody 2D"   >,
        TypeID<BoxCollider2DComponent , u8"Box Collider 2D">,
        TypeID<NativeScriptComponent  , u8"Native Script"  >
    >; // for traversing

    namespace components
    {
        template<typename Func>
        requires invocable_with_all<Func, ComponentTypes>::value
        inline void each(Func&& func)
        {
            std::apply([&](auto... type_ids) {
                (std::invoke(std::forward<Func>(func), type_ids), ...);
            }, ComponentTypes{});
            // (..., ...) ->
            // func(TypeID<A>), func(TypeID<B>), ...
        };
    };
}
