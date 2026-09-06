module;

#include <utility>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include "rke_macros.h"

export module AssetsManager;

import Types;
import HeapManager;
import Texture;
import Shader;
import Font;
import Mesh;
import Animation;
import String;
import Path;
import UUID;
import AssetAccess;

export namespace rke
{
    enum class AssetType : int
    {
        None = 0,
        Texture,
        Shader,
        Font,
        Mesh,
        Animation
    };

    using AssetData = std::unique_ptr<void, void(*)(void*)>;

// Settings(meta data); variants are cached inside each asset now
    struct RKE_API EmptySettings { uint64 padding{}; };

    union AssetSettings
    {
        EmptySettings empty;
     // ShaderSettings
     // FontSettings
     // ...
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
        // forces a re-read of the asset file next time it is loaded
        void unload_asset(AssetUUID uuid);
        // returns whether null handle or valid handle
        std::pair<AssetHandle, bool> resolve(AssetResolve& resolved, AssetUUID uuid);

        template<typename T>
        consteval AssetType get_asset_type()
        {
            if constexpr(std::is_same_v<T, Texture>)
                return AssetType::Texture;
            else if constexpr(std::is_same_v<T, Shader>)
                return AssetType::Shader;
            else if constexpr(std::is_same_v<T, Font>)
                return AssetType::Font;
            else if constexpr(std::is_same_v<T, Mesh>)
                return AssetType::Mesh;
            else if constexpr(std::is_same_v<T, Animation>)
                return AssetType::Animation;
            else return AssetType::None;
        }

        template<typename T>
        inline T* get_asset(AssetHandle handle)
            { return static_cast<T*>(get_asset_impl(handle, get_asset_type<T>())); }

        bool is_asset_loaded(AssetUUID uuid);
        bool is_handle_valid(AssetHandle handle);

        const Path& get_asset_path(AssetUUID uuid) const;
        const AssetSettings& get_asset_settings(AssetUUID uuid) const;
        AssetUUID get_asset_uuid(const Path& path) const; // expensive
    private:
        struct AssetMeta
        {
            Path asset_path{};
        // within .meta file
            AssetType type{ AssetType::None };
            AssetSettings settings{};
        // check if loaded & get from uuid
            AssetHandle handle{ asset_handle_null };
        };

        struct RuntimeAsset
        {
            AssetData data;
            AssetType type;
            uint32 version;
        };

        AssetHandle allocate_handle();
        void register_asset(AssetUUID uuid,
            const Path& path, AssetType type, AssetSettings settings);

        void* get_asset_impl(AssetHandle handle, AssetType type);
        Scope<Texture> load_texture(const AssetMeta& meta);
        Scope<Shader> load_shader(const AssetMeta& meta);
        Scope<Font> load_font(const AssetMeta& meta);
        Scope<Mesh> load_mesh(const AssetMeta& meta);
        Scope<Animation> load_animation(const AssetMeta& meta);
    private:
        std::unordered_map<AssetUUID, AssetMeta> asset_registry_{};
        std::vector<RuntimeAsset> runtime_assets_{};
        std::vector<uint32> free_asset_index_stack_{};
        std::unordered_set<UUID> failed_{};
    };
}
