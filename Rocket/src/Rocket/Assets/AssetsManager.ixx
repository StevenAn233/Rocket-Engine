module;

#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>
#include "rke_macros.h"

export module AssetsManager;

import Types;
import HeapManager;
import Texture;
import Shader;
import Font;
import String;
import Path;
import UUID;

export namespace rke
{
    using AssetUUID = UUID;
    using AssetData = std::unique_ptr<void, void(*)(void*)>;

    enum class AssetHandle : uint64 {};
    constexpr AssetHandle asset_handle_null
        { static_cast<AssetHandle>(0xFFFFFFFFFFFFFFFFull) };

    enum class AssetType : int
    {
        None = 0,
        Texture,
        Shader,
        Font
        // Mesh...
    };

    struct RKE_API EmptySettings { uint64 padding{}; };

    struct RKE_API TextureSettings
    {
        Texture::FiltFormat filt{ Texture::FiltFormat::Linear };
        Texture::WrapFormat wrap{ Texture::WrapFormat::Clamp2Edge };
        bool srgb{ true };
    };

//  struct RKE_API MeshSettings  {...}
//  struct RKE_API AudioSettings {...}

    union AssetSettings
    {
        EmptySettings empty;
        TextureSettings tex;
    };

    class RKE_API AssetsManager
    {
    public:
        AssetsManager(const Path& assets_dir);
        ~AssetsManager() = default;

        AssetsManager(const AssetsManager&) = delete;
        AssetsManager& operator=(const AssetsManager&) = delete;
        AssetsManager(AssetsManager&&) = default;
        AssetsManager& operator=(AssetsManager&&) = default;

        void clear();
        void rescan(const Path& assets_dir);

        AssetHandle load_asset(AssetUUID uuid);
        AssetUUID get_sub_uuid(AssetUUID uuid, AssetSettings settings);

        template<typename T>
        consteval AssetType get_asset_type()
        {
            if constexpr(std::is_same_v<T, Texture2D>)
                return AssetType::Texture;
            else if constexpr(std::is_same_v<T, Shader>)
                return AssetType::Shader;
            else if constexpr(std::is_same_v<T, Font>)
                return AssetType::Font;
            else return AssetType::None;
        }

        template<typename T>
        inline T* get_asset(AssetHandle handle)
            { return static_cast<T*>(get_asset_impl(handle, get_asset_type<T>())); }

        bool is_asset_loaded(AssetUUID uuid);
        bool is_handle_valid(AssetHandle handle);

        const Path& get_asset_path(AssetUUID uuid);
        const AssetSettings& get_asset_settings(AssetUUID uuid);
        AssetUUID get_asset_uuid(const Path& path);
    private:
        struct AssetMeta
        {
            Path asset_path{};
            AssetType type{ AssetType::None };
            AssetSettings settings{};

            AssetHandle handle{ asset_handle_null };
            AssetUUID parent_uuid{ 0 };
        };

        struct RuntimeAsset
        {
            AssetData data;
            AssetType type;
        };

        AssetHandle allocate_handle();
        void register_asset(AssetUUID uuid,
            const Path& path, AssetType type,
            AssetSettings settings, AssetUUID parent);

        void* get_asset_impl(AssetHandle handle, AssetType type);
        Scope<Texture2D> load_texture(const AssetMeta& meta);
        Scope<Shader> load_shader(const AssetMeta& meta);
        Scope<Font> load_font(const AssetMeta& meta);
    private:
        std::unordered_map<AssetUUID, AssetMeta> asset_registry_{};
        std::unordered_map<AssetUUID, std::vector<AssetUUID>> asset_families_{};
        std::vector<RuntimeAsset> runtime_assets_{};
        std::vector<uint32> runtime_assets_version_{};
        std::vector<uint32> free_asset_index_stack_{}; // unload not done yet
    };
}
