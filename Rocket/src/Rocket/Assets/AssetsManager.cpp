module;
module AssetsManager;

import Log;
import FileUtils;
import Application;
import HeapManager;
import ConfigProxy;

namespace {
    using namespace rke;

// Helpers
    static AssetType get_asset_type_from_extension(const Path& filepath)
    {
        if(filepath.extension().string() == u8".png")
            return AssetType::Texture;
        if(filepath.extension().string() == u8".rkshdr")
            return AssetType::Shader;
        if(filepath.extension().string() == u8".ttf")
            return AssetType::Font;
        if(filepath.extension().string() == u8".rkanim")
            return AssetType::Animation;
        return AssetType::None;
    }

    static AssetSettings extract_settings(AssetType, const ConfigReader&)
    {
        // to be implemented
        return AssetSettings{ .empty{} };
    }

    template<typename T>
    static AssetData to_asset_data(Scope<T> asset)
    {
        if(!asset) return AssetData{ nullptr, nullptr };
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
        free_asset_index_stack_.clear();
        failed_.clear();
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
            AssetType::None, AssetSettings{ .empty{} });
        
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
                register_asset(uuid, asset_path, type, settings);
            } else {
                AssetUUID new_uuid{};
                Scope<ConfigDocument> doc{ ConfigDocument::create_map() };
                doc->write(u8"UUID", new_uuid.value());
                doc->write(u8"Type", static_cast<int>(type_ext));

                doc->push_to_file(meta_path);
                CORE_INFO(u8"AssetsManager: Imported new asset '{}'.", asset_path);

                AssetSettings settings{};
                register_asset(new_uuid, asset_path, type_ext, settings);
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
            Scope<Texture> tex{ load_texture(meta) };
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
        case AssetType::Mesh: {
            Scope<Mesh> mesh{ load_mesh(meta) };
            loaded_resource = to_asset_data(std::move(mesh));
        } break;
        case AssetType::Animation: {
            Scope<Animation> clip{ load_animation(meta) };
            loaded_resource = to_asset_data(std::move(clip));
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

    void AssetsManager::unload_asset(AssetUUID uuid)
    {
        auto it{ asset_registry_.find(uuid) };
        if(it == asset_registry_.end() || uuid.empty()) return;

        AssetMeta& meta{ it->second };
        if(meta.handle == asset_handle_null) return;

        uint32 index{ extract_index(meta.handle) };
        if(index >= runtime_assets_.size())
        {
            CORE_ERROR(u8"AssetsManager: Index out of bound!");
            meta.handle = asset_handle_null;
            return;
        }
        // release the runtime slot so the asset gets re-read on next load
        runtime_assets_[index].data = nullptr;
        runtime_assets_[index].type = AssetType::None;
        runtime_assets_[index].version++; // invalidates old handles
        free_asset_index_stack_.push_back(index);
        meta.handle = asset_handle_null;
        CORE_INFO(u8"AssetsManager: Asset '{}' unloaded.", uuid.value());
    }

    std::pair<AssetHandle, bool> AssetsManager::resolve(AssetResolve& resolved, AssetUUID uuid)
    {
        if(resolved.uuid == uuid && is_handle_valid(resolved.handle))
            return { resolved.handle, false };

        resolved.uuid = uuid;
        AssetHandle handle{ load_asset(uuid) };
        if(uuid.empty() || is_handle_valid(handle))
        {
            resolved.handle = handle;
            return { resolved.handle, true };
        }
        
        resolved.handle = asset_handle_null;
        auto [_, inserted]{ failed_.insert(uuid) };
        if(inserted) CORE_ERROR(u8"AssetManager: Failed to resolve; UUID '{}' invalid!", uuid.value());
        return { resolved.handle, true };
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
        for(const auto& [uuid, meta] : asset_registry_) // expensive
            if(meta.asset_path == path) return uuid;
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
            ++runtime_assets_[index_val].version;
            free_asset_index_stack_.pop_back();
        }
        // downside cast should be fine because I don't think someone can load billions of assets...
        uint64 version_val{ static_cast<uint64>(runtime_assets_[index_val].version) };
        return static_cast<AssetHandle>((version_val << 32) | (index_val & 0xFFFFFFFFull));
    }

    void AssetsManager::register_asset(AssetUUID uuid,
        const Path& path, AssetType type, AssetSettings settings)
    {
        if(asset_registry_.contains(uuid)) {
            CORE_ERROR(u8"AssetsManager: UUID collision "
                u8"or duplicate registration for '{}'!", uuid.value());
            return;
        }
        asset_registry_.emplace(uuid, AssetMeta(path, type, settings, asset_handle_null));
    }

    void* AssetsManager::get_asset_impl(AssetHandle handle, AssetType type)
    {
        if(!is_handle_valid(handle)) return nullptr;
        RuntimeAsset& asset{ runtime_assets_[extract_index(handle)] };
        if(asset.type != type) return nullptr;
        return asset.data.get();
    }

    Scope<Texture> AssetsManager::load_texture(const AssetMeta& meta)
    {
        const Path& path{ meta.asset_path };
        if(path.empty() || !path.exists()) return nullptr;
        return create_scope<Texture>(path);
    }

    Scope<Shader> AssetsManager::load_shader(const AssetMeta& meta)
    {
        const Path& path{ meta.asset_path };
        if(path.empty() || !path.exists()) return nullptr;
        return create_scope<Shader>(path);
    }

    Scope<Font> AssetsManager::load_font(const AssetMeta& meta)
    {
        const Path& path{ meta.asset_path };
        if(path.empty() || !path.exists()) return nullptr;
        return create_scope<Font>(path);
    }

    Scope<Mesh> AssetsManager::load_mesh(const AssetMeta& meta)
    {
        // to be implemented
        return nullptr;
    }

    Scope<Animation> AssetsManager::load_animation(const AssetMeta& meta)
    {
        const Path& path{ meta.asset_path };
        if(path.empty() || !path.exists()) return nullptr;

        Scope<Animation> anim{ create_scope<Animation>() };
        if(!anim->load_from(path)) return nullptr;
        return anim;
    }
}
