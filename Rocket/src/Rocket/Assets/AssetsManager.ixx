module;

#include <variant>
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

    struct RKE_API EmptySettings {};

    struct RKE_API TextureSettings
    {
        Texture::FiltFormat filt{ Texture::FiltFormat::Linear };
        Texture::WrapFormat wrap{ Texture::WrapFormat::Clamp2Edge };
        bool srgb{ true };
    };

//  struct RKE_API MeshSettings  {...}
//  struct RKE_API AudioSettings {...}

    using AssetSettings = std::variant <
        EmptySettings,
        TextureSettings
    //  MeshSettings, etc.
    >;

    struct AssetMeta
    {
        Path path{};
        AssetType type{ AssetType::None };
        AssetSettings settings{};
        AssetHandle handle{}; // 0 for invalid/empty

        AssetUUID parent_uuid{ 0 };
    };

    class RKE_API AssetsManager
    {
    public:
        static void scan_assets_directory(const Path& root_dir);
        static AssetUUID get_or_create_sub_uuid(AssetUUID uuid,
                                                const AssetSettings& settings);

        static AssetHandle load_asset(AssetUUID uuid);
        template<typename T>
        static T* get_asset(AssetHandle handle)
        {
            void* raw_ptr{ get_asset_internal(handle) };
            if(!raw_ptr) return nullptr;
            return static_cast<T*>(raw_ptr);
        }

        static bool is_asset_loaded(AssetUUID uuid);
        static bool is_handle_valid(AssetHandle handle);

        static const Path& get_asset_path(AssetUUID uuid);
        static const AssetSettings& get_asset_settings(AssetUUID uuid);
        static AssetUUID get_asset_uuid(const Path& path);
    private:
        static void* get_asset_internal(AssetHandle handle);
        static void register_asset(AssetUUID uuid, const Path& path, AssetType type,
                                   AssetSettings settings, AssetUUID parent);

        static Ref<Texture2D> load_texture(const AssetMeta& meta);
        static Ref<Shader> load_shader(const AssetMeta& meta);
        static Ref<Font> load_font(const AssetMeta& meta);
    };
}
