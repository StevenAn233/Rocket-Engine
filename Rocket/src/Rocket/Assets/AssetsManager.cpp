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
    struct SettingsHashers
    {
        Size operator()(const EmptySettings&) const { return 0ull; }
        Size operator()(const TextureSettings& s) const
        {
            Size h1{ std::hash<int >{}(static_cast<int>(s.filt)) };
            Size h2{ std::hash<int >{}(static_cast<int>(s.wrap)) };
            Size h3{ std::hash<bool>{}(s.srgb) };
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    static Size get_settings_hash(AssetType type, const AssetSettings& settings)
    {
        switch(type)
        {
        case AssetType::Texture: return SettingsHashers()(settings.tex);
        default: return SettingsHashers()(settings.empty);
        }
    }

// Helpers
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

    static AssetSettings extract_settings(AssetType type, const ConfigReader& reader)
    {
        AssetSettings settings{ .empty{} };
        switch(type)
        {
        case AssetType::Texture:
        {
            Scope<ConfigReader> settings_reader{ reader.get_child(u8"Settings") };
            if(settings_reader) {
                TextureSettings& tex_settings{ settings.tex };
                tex_settings.filt = static_cast<Texture::FiltFormat>
                    (settings_reader->get_at(u8"Filt", 0ui32));
                tex_settings.wrap = static_cast<Texture::WrapFormat>
                    (settings_reader->get_at(u8"Wrap", 0ui32));
                tex_settings.srgb = settings_reader->get_at(u8"sRGB", true);
            }
        } break;
        default: break;
        }
        return settings;
    }

    static void write_settings(AssetType type, const AssetSettings& settings, ConfigDocument& doc)
    {
        if(!doc.is_map()) {
            CORE_ERROR(u8"AssetsManager: Doc is not a map!");
            return;
        }
        switch(type) {
        case AssetType::Texture: {
            const TextureSettings& tex_settings{ settings.tex };
            Scope<ConfigDocument> writer{ doc.get_child(u8"Settings") };
            if(!writer) break;
            writer->write(u8"Filt", static_cast<int>(tex_settings.filt));
            writer->write(u8"Wrap", static_cast<int>(tex_settings.wrap));
            writer->write(u8"sRGB", tex_settings.srgb);
        } break;
        default: break;
        }
    }

    template<typename T>
    static AssetData to_asset_data(Scope<T> asset)
    {
        return AssetData(asset.release(),
            [](void* ptr) { delete static_cast<T*>(ptr); } );
    }

    static constexpr uint32 extract_index(AssetHandle handle)
        { return static_cast<uint32>(static_cast<uint64>(handle)); }
    static constexpr uint32 extract_version(AssetHandle handle)
        { return static_cast<uint32>(static_cast<uint64>(handle) >> 32); }
}

namespace rke
{
    AssetsManager::AssetsManager(const Path& assets_dir) { rescan(assets_dir); }

    void AssetsManager::clear()
    {
        asset_registry_.clear();
        asset_families_.clear();
        free_asset_index_stack_.clear();
        for(Size i{}; i < runtime_assets_.size(); i++)
        {
            runtime_assets_[i].data = nullptr;
            runtime_assets_[i].type = AssetType::None;
            free_asset_index_stack_.push_back(static_cast<uint32>(i));
            // don't clear version info here!
        }
    }

    void AssetsManager::rescan(const Path& assets_dir)
    {
        clear();
    // 1. scan to extract asset/meta path
        std::vector<Path> all_files{};
        std::vector<Path> all_metas{};
        try {
            for(const auto& entry : fs::recursive_directory_iterator(assets_dir.get()))
            {
                if(!entry.is_regular_file()) continue;
                
                if(entry.path().extension() == ".meta")
                    all_metas.push_back(entry.path());
                else all_files.push_back(entry.path());
            }
        } catch(const std::exception& e) {
            CORE_ERROR(u8"AssetsManager: Failed to scan directory '{}': {}!",
                assets_dir, e.what());
            return;
        }

    // 2. register UUIDs according to meta paths
        register_asset(AssetUUID(0), Path(u8"<Empty Asset>"),
            AssetType::None, AssetSettings{ .empty{} }, AssetUUID(0));
        
        std::unordered_set<String> valid_meta_paths{};
        for(const Path& asset_path : all_files)
        {
            AssetType type_ext{ get_asset_type_from_extension(asset_path) };
            if(type_ext == AssetType::None) continue;

            Path meta_path{ asset_path.string() + u8".meta" };
            valid_meta_paths.insert(meta_path.string());
            if(meta_path.exists()) {
                Scope<ConfigReader> reader{ ConfigReader::create(meta_path) };
                AssetUUID uuid{ reader->get_at(u8"UUID", 0ui64) };
                AssetType type{ static_cast<AssetType>(reader->get_at(u8"Type", 0)) };

                AssetSettings settings{ extract_settings(type, *reader) };
                register_asset(uuid, asset_path, type, settings, uuid);
                asset_families_[uuid].push_back(uuid); // requires to store itself!

                Scope<ConfigReader> sub_assets_reader{ reader->get_child(u8"SubAssets") };
                if(sub_assets_reader && sub_assets_reader->is_array())
                {
                    std::vector<AssetUUID>& sub_uuids{ asset_families_[uuid] };
                    sub_assets_reader->for_each([&](Scope<ConfigReader> reader)
                    {
                        if(!reader) return;
                        AssetUUID sub_uuid{ reader->get_at(u8"UUID", 0ui64) };
                        if(sub_uuid.empty()) return;
                        sub_uuids.push_back(sub_uuid);
                        AssetSettings sub_settings{ extract_settings(type, *reader) };
                        register_asset(sub_uuid, asset_path, type, sub_settings, uuid);
                    });
                }
            } else {
                AssetUUID new_uuid{};
                Scope<ConfigDocument> doc{ ConfigDocument::create_map() };
                doc->write(u8"UUID", new_uuid.value());
                doc->write(u8"Type", static_cast<int>(type_ext));

                AssetSettings settings{};
                if(type_ext == AssetType::Texture) {
                    settings.tex = TextureSettings{};
                    write_settings(type_ext, settings, *doc);
                } // to optimize later

                doc->push_to_file(meta_path);
                CORE_INFO(u8"AssetsManager: Imported new asset '{}'.", asset_path);

                register_asset(new_uuid, asset_path, type_ext, settings, new_uuid);
            }
        }

    // 3. clean orphan meta files
        for(const Path& meta_path : all_metas)
        {
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

    AssetHandle AssetsManager::load_asset(AssetUUID uuid)
    {
        if(uuid.empty()) return asset_handle_null;

        // check registry
        auto it{ asset_registry_.find(uuid) };
        if(it == asset_registry_.end()) {
            CORE_ERROR(u8"AssetsManager: Unknown Asset UUID '{}'! "
                u8"Did you forget to register it?", uuid.value());
            return asset_handle_null;
        }

        // check handle
        AssetMeta& meta{ it->second };
        if(meta.handle != asset_handle_null) return meta.handle;

        AssetData loaded_resource{ nullptr, nullptr };
        CORE_INFO(u8"AssetsManager: Loading asset '{}' from '{}'...",
            uuid.value(), meta.asset_path);
        switch(meta.type)
        {
        case AssetType::Texture: {
            Scope<Texture2D> tex{ load_texture(meta) };
            loaded_resource = to_asset_data(std::move(tex));
        } break;
        case AssetType::Shader: {
            Scope<Shader> shader{ load_shader(meta) };
            loaded_resource = to_asset_data(std::move(shader));
        } break;
        case AssetType::Font: {
            Scope<Font> font{ load_font(meta) };
            loaded_resource = to_asset_data(std::move(font));
        } break;
        default:
            CORE_ERROR(u8"AssetsManager: Unknown asset type!");
            return asset_handle_null;
        }

        if(!loaded_resource) {
            CORE_ERROR(u8"AssetsManager: "
                u8"Failed to load asset at '{}'", meta.asset_path);
            return asset_handle_null;
        }
        CORE_INFO(u8"AssetsManager: Asset '{}' successfully loaded.", uuid.value());

        AssetHandle handle{ allocate_handle() };
        RuntimeAsset& asset{ runtime_assets_[extract_index(handle)] };
        
        asset.data = std::move(loaded_resource);
        asset.type = meta.type;

        meta.handle = handle;
        return handle;
    }

    AssetUUID AssetsManager::get_sub_uuid(AssetUUID uuid, AssetSettings settings)
    {
        AssetUUID main_uuid{ asset_registry_.at(uuid).parent_uuid };
        auto it{ asset_registry_.find(main_uuid) };
        if(it == asset_registry_.end()) {
            CORE_ERROR(u8"AssetsManager: UUID '{}' not found!", main_uuid.value());
            return AssetUUID(0);
        }

        const AssetMeta& main_meta{ it->second };
    // Find sub_uuid
        Size desired_hash{ get_settings_hash(main_meta.type, settings) };
        if(asset_families_.contains(main_uuid)) {
            for(AssetUUID sub_uuid : asset_families_[main_uuid])
            {
                const AssetMeta& sub_meta{ asset_registry_.at(sub_uuid) };
                CORE_ASSERT(main_meta.type == sub_meta.type,
                    u8"AssetManager: Sub asset must be as the same type as its main asset!");
                if(get_settings_hash(main_meta.type, sub_meta.settings) == desired_hash)
                    return sub_uuid;
            }
        }

    // Create sub_uuid
        AssetUUID new_sub_uuid{};
        String meta_path{ main_meta.asset_path.string() + u8".meta" };

        Scope<ConfigDocument> doc{ ConfigDocument::create(meta_path) };
        CORE_ASSERT(doc, u8"AssetsManager: Doc null!");
        Scope<ConfigDocument> sub_assets{ doc->get_child(u8"SubAssets") };
        CORE_ASSERT(sub_assets, u8"AssetsManager: Sub-assets null!");

        Scope<ConfigDocument> new_sub_asset{ sub_assets->push_map() };
        CORE_ASSERT(new_sub_asset, u8"AssetsManager: Failed to create sub-asset!");
        new_sub_asset->write(u8"UUID", new_sub_uuid.value());
        write_settings(main_meta.type, settings, *new_sub_asset);

        doc->push_to_file(meta_path);
        CORE_INFO(u8"AssetsManager: Created sub-asset '{}' for '{}'.",
            new_sub_uuid.value(), main_meta.asset_path);

        register_asset(new_sub_uuid, main_meta.asset_path, main_meta.type, settings, main_uuid);
        asset_families_[main_uuid].push_back(new_sub_uuid);
        return new_sub_uuid;
    }

    bool AssetsManager::is_asset_loaded(AssetUUID uuid)
    {
        auto it{ asset_registry_.find(uuid) };
        if(it == asset_registry_.end()) return false;
        return it->second.handle != asset_handle_null;
    }

    bool AssetsManager::is_handle_valid(AssetHandle handle)
    {
        if(handle == asset_handle_null) return false;
        uint32 index{ extract_index(handle) };
        return index < runtime_assets_.size()
            && runtime_assets_[index].data.get() != nullptr
            && runtime_assets_[index].version == extract_version(handle);
    }

    const Path& AssetsManager::get_asset_path(AssetUUID uuid)
    {
        auto it{ asset_registry_.find(uuid) };
        if(it == asset_registry_.end()) {
            CORE_ERROR(u8"AssetsManager: Asset uuid '{}' not found!", uuid.value());
            it = asset_registry_.find(0);
        }
        return it->second.asset_path;
    }

    const AssetSettings& AssetsManager::get_asset_settings(AssetUUID uuid)
    {
        auto it{ asset_registry_.find(uuid) };
        if(it == asset_registry_.end()) {
            CORE_ERROR(u8"AssetsManager: Asset uuid '{}' not found!", uuid.value());
            it = asset_registry_.find(0);
        }
        return it->second.settings;
    }

    AssetUUID AssetsManager::get_asset_uuid(const Path& path)
    {
        for(const auto& [uuid, meta] : asset_registry_)
            if((meta.asset_path == path) && (meta.parent_uuid == uuid))
                return uuid;
        return AssetUUID(0);
    }

// private
    AssetHandle AssetsManager::allocate_handle()
    {
        uint64 index_val{};
        if(free_asset_index_stack_.empty()) {
            runtime_assets_.push_back(RuntimeAsset
                { AssetData(nullptr, nullptr), AssetType::None, 0 });
            index_val = runtime_assets_.size() - 1;
        } else {
            index_val = static_cast<uint64>(free_asset_index_stack_.back());
            free_asset_index_stack_.pop_back();
        }
        uint64 version_val{ static_cast<uint64>(++runtime_assets_[index_val].version) };
        // downside cast should be fine because I don't think someone can load billions of assets...
        return static_cast<AssetHandle>((version_val << 32) | (index_val & 0xFFFFFFFFull));
    }

    void AssetsManager::register_asset(AssetUUID uuid,
        const Path& path, AssetType type, AssetSettings settings, AssetUUID parent)
    {
        if(asset_registry_.contains(uuid)) {
            CORE_ERROR(u8"AssetsManager: UUID collision "
                u8"or duplicate registration for '{}'!", uuid.value());
            return;
        }
        asset_registry_.emplace(uuid, AssetMeta(path, type, settings, asset_handle_null, parent));
    }

    void* AssetsManager::get_asset_impl(AssetHandle handle, AssetType type)
    {
        if(!is_handle_valid(handle)) return nullptr;
        RuntimeAsset& asset{ runtime_assets_[extract_index(handle)] };
        if(asset.type != type) return nullptr;
        return asset.data.get();
    }

    Scope<Texture2D> AssetsManager::load_texture(const AssetMeta& meta)
    {
        const Path& path{ meta.asset_path };
        if(path.empty() || !path.exists()) return nullptr;

        const TextureSettings& settings{ meta.settings.tex };
        return Texture2D::create(path, settings.filt, settings.wrap, settings.srgb);
    }

    Scope<Shader> AssetsManager::load_shader(const AssetMeta& meta)
    {
        const Path& path{ meta.asset_path };
        if(path.empty() || !path.exists()) return nullptr;
        return Shader::create(path);
    }

    Scope<Font> AssetsManager::load_font(const AssetMeta& meta)
    {
        const Path& path{ meta.asset_path };
        if(path.empty() || !path.exists()) return nullptr;
        return create_scope<Font>(path);
    }
}
