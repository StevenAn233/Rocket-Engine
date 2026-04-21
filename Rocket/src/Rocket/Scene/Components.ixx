module;

#include <utility>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rke_macros.h"

export module Components;

import Types;
import String;
import HeapManager;
import SceneCamera;
import PhysicsLayers;
import Texture;
import UUID;
import AssetsManager;

// put datas that you wanna deal with simultaneously in the same component
export namespace rke
{
// MUST OWNED
    struct RKE_API UUIDComponent
    {
        UUID uuid{};

        UUIDComponent() = default;
        UUIDComponent(UUID specified) : uuid(specified) {}
        UUIDComponent(const UUIDComponent&) = default;
    };

    struct RKE_API TagComponent
    {
        String tag{ u8"None" };

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(String str) : tag(std::move(str)) {}
    };

    struct RKE_API TransformComponent
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 rotation{ 0.0f };
        glm::vec3 size	  { 1.0f };

        bool locked{ false };

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 siz)
            : position(std::move(pos))
            , rotation(std::move(rot))
            , size	  (std::move(siz)) {}

        glm::mat4 get_transform() const
        {
            return glm::translate(glm::mat4{ 1.0f }, position)
                 * glm::mat4_cast(glm::quat(glm::radians(rotation)))
                 * glm::scale	 (glm::mat4{ 1.0f }, size);
        }
    };
// ---

// Rendering(Confilct! Can only own one of them at most!)
    struct RKE_API SpriteComponent
    {
        enum class BlendingMode : uint32
        {
            Opaque = 0,
            Cutout,
            Transparent
        };

        struct RKE_API TextureAsset
        {
            AssetUUID uuid{ 0 };
            AssetHandle handle{ 0 };

            float tiling_factor{ 1.0f };
            glm::vec2 cell_pixels{ 1.0f, 1.0f };
            glm::vec2 cell_coords{ 0.0f, 0.0f }; // left-bottom (0, 0)
            glm::vec2 cell_counts{ 1.0f, 1.0f };

            TextureAsset() = default;
            TextureAsset(AssetUUID id) : uuid(id) {}

            bool has_asset() const { return !uuid.empty(); }
            bool is_loaded() const { return  handle != 0;  }
        };

        TextureAsset texture{};

        glm::vec4 color{ 1.0f };
        BlendingMode blending_mode{ BlendingMode::Opaque };
        int rendering_layer{ 0 };

        SpriteComponent() = default;
        SpriteComponent(AssetUUID uuid) : texture(uuid) {}
        SpriteComponent(const SpriteComponent&) = default;
    };

    struct RKE_API CameraComponent
    {
        SceneCamera camera {};
        bool master	{ false }; // MAYBE MOVE TO SCENE
        bool aspect_ratio_fixed{ false };

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
    };

    // struct RKE_API MeshComponent {...};
// ---

// Requires to have SpriteComponent!
    struct RKE_API Rigidbody2DComponent
    {
        enum class BodyType : uint32
        {
            Static = 0,
            Dynamic,
            Kinematic
        };

        BodyType type{ BodyType::Static };
        bool rotation_fixed{ false };

        uint64 body_id{};

        Rigidbody2DComponent() = default;
        Rigidbody2DComponent(const Rigidbody2DComponent& other)
            : type(other.type)
            , rotation_fixed(other.rotation_fixed) {}
    };

    struct RKE_API BoxCollider2DComponent
    {
        uint8 layer_index{ 0 }; // 0 for default

        glm::vec2 offset{ 0.0f, 0.0f };
        glm::vec2 size  { 0.5f, 0.5f }; // half w, half h (0.0 to 1.0)

        float density	 { 1.0f };
        float friction	 { 0.5f };
        float restitution{ 0.0f }; // 'bounciness'

        uint64 shape_id{};

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent& other)
            : layer_index(other.layer_index)
            , offset	 (other.offset	   )
            , size		 (other.size	   )
            , density	 (other.density	   )
            , friction	 (other.friction   )
            , restitution(other.restitution) {}
    };
// ---

    struct RKE_API NativeScriptComponent
    {
        String script_name{};
        bool wants_to_update{ true };

        NativeScriptComponent() = default;
        NativeScriptComponent(const NativeScriptComponent&) = default;
    };
}
