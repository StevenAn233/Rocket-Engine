module;

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
    using AssetHandle = uint32;
    using AssetUUID   = UUID;

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
        static void scan_assets_directory(const Path& root_dir);
        static AssetUUID get_sub_uuid(AssetUUID uuid, AssetSettings settings);

        static AssetHandle load_asset(AssetUUID uuid);

        template<typename T>
        static consteval AssetType get_asset_type()
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
        static T* get_asset(AssetHandle handle)
        {
            return static_cast<T*>
                (get_asset_internal(handle, get_asset_type<T>()));
        }

        static bool is_asset_loaded(AssetUUID uuid);
        static bool is_handle_valid(AssetHandle handle);

        static const Path& get_asset_path(AssetUUID uuid);
        static const AssetSettings& get_asset_settings(AssetUUID uuid);
        static AssetUUID get_asset_uuid(const Path& path);
    private:
        static void* get_asset_internal(AssetHandle handle, AssetType type);
    };
}
