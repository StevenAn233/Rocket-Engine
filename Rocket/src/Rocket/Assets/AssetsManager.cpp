module;
module AssetsManager;

import Log;
import FileUtils;
import Application;
import HeapManager;
import ConfigProxy;

namespace {
    using namespace rke;

// Hash support(for sub-asset)
    struct TextureSettingsHasher {
        Size operator()(const TextureSettings& s) const
        {
            Size h1{ std::hash<int >{}(static_cast<int>(s.filt)) };
            Size h2{ std::hash<int >{}(static_cast<int>(s.wrap)) };
            Size h3{ std::hash<bool>{}(s.srgb) };
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    struct SettingsHasherVisitor {
        Size operator()(const EmptySettings&) const { return 0; }
        Size operator()(const TextureSettings& s) const
            { return TextureSettingsHasher{}(s); }
    };

    static Size get_settings_hash(const AssetSettings& settings)
        { return std::visit(SettingsHasherVisitor{}, settings); }

// Static data/registry
    static std::unordered_map<AssetUUID, AssetMeta> s_asset_registry{};
    static std::unordered_map<AssetUUID, std::vector<AssetUUID>> s_asset_families{};

    struct RuntimeAssetSlot
    {
        Ref<void> asset_ref;
        void* raw_pointer{ nullptr };
        AssetType type{ AssetType::None };
    };

    static std::vector<RuntimeAssetSlot> s_runtime_assets{};

// Helpers
    static AssetHandle allocate_handle()
    {
        if(s_runtime_assets.empty())
            s_runtime_assets.emplace_back(); // Dummy for Handle 0
    
        s_runtime_assets.emplace_back();
        return static_cast<AssetHandle>(s_runtime_assets.size() - 1);
    }

    static AssetType get_asset_type_from_extension(const Path& filepath)
    {
        if(filepath.extension().string() == u8".png")
            return AssetType::Texture;
        if(filepath.extension().string() == u8".rkshdr")
            return AssetType::Shader;
        if(filepath.extension().string() == u8".ttf")
            return AssetType::Font;
        return AssetType::None;
    }

    static AssetSettings extract_settings(AssetType type, const ConfigReader* reader)
    {
        AssetSettings settings{ EmptySettings{} };
        switch(type)
        {
        case AssetType::Texture: {
            Scope<ConfigReader> settings_reader{ reader->get_child(u8"Settings") };
            if(settings_reader) {
                TextureSettings tex_settings{};
                tex_settings.filt = static_cast<Texture::FiltFormat>
                    (settings_reader->get_at(u8"Filt", 0ui32));
                tex_settings.wrap = static_cast<Texture::WrapFormat>
                    (settings_reader->get_at(u8"Wrap", 0ui32));
                tex_settings.srgb = settings_reader->get_at(u8"sRGB", true);
                settings = tex_settings;
            }
        } break;
        default: break;
        }
        return settings;
    }

    static void push_settings(AssetType type,
        const AssetSettings& settings, ConfigDocument* doc)
    {
        if(!doc->is_map()) {
            CORE_ERROR(u8"AssetsManager: Doc is not a map!");
            return;
        }
        switch(type) {
        case AssetType::Texture: {
            auto* tex_settings{ std::get_if<TextureSettings>(&settings) };
            CORE_ASSERT(tex_settings, u8"AssetsManager: "
                u8"Asset type(Texture) doesn't match with settings!");

            Scope<ConfigDocument> settings{ doc->get_child(u8"Settings") };
            if(!settings) break;
            settings->write(u8"Filt", static_cast<int>(tex_settings->filt));
            settings->write(u8"Wrap", static_cast<int>(tex_settings->wrap));
            settings->write(u8"sRGB", tex_settings->srgb);
        } break;
        default: break;
        }
    }
}

namespace rke
{
    void AssetsManager::scan_assets_directory(const Path& root_dir)
    {
        s_asset_registry.clear();
        s_asset_families.clear();
        s_runtime_assets.clear();

    // 1. scan to extract asset/meta path
        std::vector<Path> all_files{};
        std::vector<Path> all_metas{};
        try {
            for(const auto& entry : fs::recursive_directory_iterator(root_dir.get()))
            {
                if(!entry.is_regular_file()) continue;
                
                if(entry.path().extension() == ".meta")
                    all_metas.push_back(entry.path());
                else all_files.push_back(entry.path());
            }
        } catch(const std::exception& e) {
            CORE_ERROR(u8"AssetsManager: Failed to scan directory '{}': {}!",
                root_dir, e.what());
            return;
        }

    // 2. register UUIDs according to meta paths
        std::unordered_set<String> valid_meta_paths{};
        for(const Path& file_path : all_files)
        {
            AssetType type_ext{ get_asset_type_from_extension(file_path) };
            if(type_ext == AssetType::None) continue;

            Path meta_path{ file_path.string() + u8".meta" };
            valid_meta_paths.insert(meta_path.string());
            if(meta_path.exists()) {
                Scope<ConfigReader> reader{ ConfigReader::create(meta_path) };
                AssetUUID uuid{ reader->get_at(u8"UUID", 0ui64) };
                AssetType type{ static_cast<AssetType>(reader->get_at(u8"Type", 0)) };

                AssetSettings settings{ extract_settings(type, reader.get()) };
                register_asset(uuid, file_path, type, settings, uuid);
                s_asset_families[uuid].push_back(uuid);

                Scope<ConfigReader> sub_assets_reader{ reader->get_child(u8"SubAssets") };
                if(sub_assets_reader && sub_assets_reader->is_array())
                {
                    std::vector<AssetUUID>& sub_uuids{ s_asset_families[uuid] };
                    sub_assets_reader->for_each_arr([&](Scope<ConfigReader> sub_asset)
                    {
                        AssetUUID sub_uuid{ sub_asset->get_at(u8"UUID", 0ui64) };
                        if(sub_uuid.empty()) return;
                        sub_uuids.push_back(sub_uuid);
                        AssetSettings sub_settings{ extract_settings(type, sub_asset.get()) };
                        register_asset(sub_uuid, file_path, type, sub_settings, uuid);
                    });
                }
            } else {
                AssetUUID new_uuid{};
                Scope<ConfigDocument> doc{ ConfigDocument::create_map() };
                doc->write(u8"UUID", new_uuid.value());
                doc->write(u8"Type", static_cast<int>(type_ext));

                AssetSettings settings{};
                if(type_ext == AssetType::Texture) {
                    settings = TextureSettings{};
                    push_settings(type_ext, settings, doc.get());
                } // to optimize later

                doc->push_to_file(meta_path);
                CORE_INFO(u8"AssetsManager: Imported new asset '{}'.", file_path);

                register_asset(new_uuid, file_path, type_ext, settings, new_uuid);
            }
        }

    // 3. clean orphan metas
        for(const Path& meta_path : all_metas) {
            if(valid_meta_paths.contains(meta_path.string())) continue;
            try {
                fs::remove(meta_path.get());
            } catch(const std::exception& e) {
                CORE_ERROR(u8"AssetsManager: Failed to delete orphan meta '{}': {}!",
                    meta_path, e.what());
            }
            CORE_WARN(u8"AssetsManager: Deleted orphan meta file '{}'.", meta_path);
        }
    }

    AssetUUID AssetsManager::get_or_create_sub_uuid
        (AssetUUID uuid, const AssetSettings& settings)
    {
        AssetUUID main_uuid{ s_asset_registry.at(uuid).parent_uuid };

        auto it{ s_asset_registry.find(main_uuid) };
        CORE_ASSERT(it != s_asset_registry.end(),
            u8"AssetsManager: Source asset '{}' not found!", main_uuid.value());

    // Find sub_uuid
        Size desired_hash{ get_settings_hash(settings) };
        if(s_asset_families.contains(main_uuid)) {
            for(AssetUUID sub_uuid : s_asset_families[main_uuid]) {
                const AssetMeta& meta{ s_asset_registry.at(sub_uuid) };
                if(get_settings_hash(meta.settings) == desired_hash)
                    return sub_uuid;
            }
        }

    // Create sub_uuid
        AssetUUID new_sub_uuid{};
        const AssetMeta& main_meta{ it->second };
        String meta_path{ main_meta.path.string() + u8".meta" };

        Scope<ConfigDocument> doc{ ConfigDocument::create(meta_path) };
        CORE_ASSERT(doc, u8"AssetsManager: Doc null!");
        Scope<ConfigDocument> sub_assets{ doc->get_child(u8"SubAssets") };
        CORE_ASSERT(sub_assets, u8"AssetsManager: Sub-assets null!");

        Scope<ConfigDocument> new_sub_asset{ sub_assets->push_map() };
        CORE_ASSERT(new_sub_asset, u8"AssetsManager: Failed to create sub-asset!");
        new_sub_asset->write(u8"UUID", new_sub_uuid.value());
        push_settings(main_meta.type, settings, new_sub_asset.get());

        doc->push_to_file(meta_path);
        CORE_INFO(u8"AssetsManager: Created sub-asset '{}' for '{}'.",
            new_sub_uuid.value(), main_meta.path);

        register_asset(new_sub_uuid, main_meta.path, main_meta.type,
                       settings, main_uuid);
        s_asset_families[main_uuid].push_back(new_sub_uuid);
        return new_sub_uuid;
    }

    bool AssetsManager::is_asset_loaded(AssetUUID uuid)
    {
        auto it{ s_asset_registry.find(uuid) };
        if(it == s_asset_registry.end()) return false;
        return it->second.handle != 0;
    }

    bool AssetsManager::is_handle_valid(AssetHandle handle)
    {
        return handle > 0 && handle < s_runtime_assets.size()
            && s_runtime_assets[handle].raw_pointer != nullptr;
    }

    const Path& AssetsManager::get_asset_path(AssetUUID uuid)
    {
        auto it{ s_asset_registry.find(uuid) };
        CORE_ASSERT(it != s_asset_registry.end(), u8"AssetsManager: Asset path not found!");
        return it->second.path;
    }

    const AssetSettings& AssetsManager::get_asset_settings(AssetUUID uuid)
    {
        auto it{ s_asset_registry.find(uuid) };
        CORE_ASSERT(it != s_asset_registry.end(),
            u8"AssetsManager: UUID '{}' not found!", uuid.value());
        return it->second.settings;
    }

    AssetUUID AssetsManager::get_asset_uuid(const Path& path)
    {
        for(const auto& [uuid, meta] : s_asset_registry)
            if((meta.path == path) && (meta.parent_uuid == uuid))
                return uuid;
        return AssetUUID(0);
    }

    AssetHandle AssetsManager::load_asset(AssetUUID uuid)
    {
        if(uuid.empty()) return 0;

        // check registry
        auto it{ s_asset_registry.find(uuid) };
        if(it == s_asset_registry.end()) {
            CORE_ERROR(u8"AssetsManager: Unknown Asset UUID '{}'! "
                u8"Did you forget to register it?", uuid.value());
            return 0;
        }

        // check handle
        AssetMeta& meta{ it->second };
        if(meta.handle != 0) return meta.handle;

        Ref<void> loaded_resource{};
        void* raw_ptr{};

        CORE_INFO(u8"AssetsManager: Loading asset '{}' from '{}'...",
            uuid.value(), meta.path);
        switch(meta.type)
        {
        case AssetType::Texture: {
            auto tex{ load_texture(meta) };
            loaded_resource = std::static_pointer_cast<void>(tex);
            raw_ptr = tex.get();
        } break;
        case AssetType::Shader: {
            auto sh{ load_shader(meta) };
            loaded_resource = std::static_pointer_cast<void>(sh);
            raw_ptr = sh.get();
        } break;
        case AssetType::Font: {
            auto font{ load_font(meta) };
            loaded_resource = std::static_pointer_cast<void>(font);
            raw_ptr = font.get();
        } break;
        default:
            CORE_ERROR(u8"AssetsManager: Unknown asset type!");
            return 0;
        }

        if(!loaded_resource) {
            CORE_ERROR(u8"AssetsManager: Failed to load asset at '{}'", meta.path);
            return 0;
        }
        CORE_INFO(u8"AssetsManager: Asset '{}' successfully loaded.", uuid.value());

        AssetHandle handle{ allocate_handle() };
        RuntimeAssetSlot& slot{ s_runtime_assets[handle] };
        
        slot.asset_ref   = loaded_resource;
        slot.raw_pointer = raw_ptr;
        slot.type        = meta.type;

        meta.handle = handle;
        return handle;
    }

// private
    void* AssetsManager::get_asset_internal(AssetHandle handle)
    {
        if(!is_handle_valid(handle)) return nullptr;
        return s_runtime_assets[handle].raw_pointer;
    }

    void AssetsManager::register_asset(AssetUUID uuid, const Path& path, AssetType type,
                                       AssetSettings settings, AssetUUID parent)
    {
        if(s_asset_registry.contains(uuid)) {
            CORE_ERROR(u8"AssetsManager: UUID collision "
                u8"or duplicate registration for '{}'!", uuid.value());
            return;
        }
        s_asset_registry[uuid] = { path, type, settings, 0, parent };
    }

    Ref<Texture2D> AssetsManager::load_texture(const AssetMeta& meta)
    {
        const Path& path{ meta.path };
        if(path.empty() || !path.exists()) return nullptr;

        const auto* settings{ std::get_if<TextureSettings>(&meta.settings) };
        CORE_ASSERT(settings, u8"AssetsManager: Texture settings invalid!");

        return Texture2D::create(path, settings->filt, settings->wrap, settings->srgb);
    }

    Ref<Shader> AssetsManager::load_shader(const AssetMeta& meta)
    {
        const Path& path{ meta.path };
        if(path.empty() || !path.exists()) return nullptr;
        return Shader::create(path);
    }

    Ref<Font> AssetsManager::load_font(const AssetMeta& meta)
    {
        const Path& path{ meta.path };
        if(path.empty() || !path.exists()) return nullptr;
        return create_ref<Font>(path);
    }
}
