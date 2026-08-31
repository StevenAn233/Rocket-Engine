module;
module Project;

import Log;
import FileUtils;
import ConfigProxy;

namespace rke
{
    String Project::s_cmake_lists_txt{};
    String Project::s_cmake_presets_json{};
    String Project::s_myscript_cpp{};
    String Project::s_editorconfig{};

    Project::Project(const Path& rkproj_path)
        : rkproj_path_(rkproj_path), project_dir_(rkproj_path.parent_path())
    {
        Scope<ConfigReader> reader{ ConfigReader::create(rkproj_path) };
        auto& config{ project_config_ };
        config.name = reader->get_at(u8"Project", String{});
        if(config.name.empty()) {
            CORE_ERROR(u8"Project: Invalid project file format in '{}'!", rkproj_path);
            return;
        }
        
        Scope<ConfigReader> config_reader{ reader->get_child(u8"Config") };
        if(!config_reader)
            CORE_ERROR(u8"Project: 'Config' not found in '{}'!", rkproj_path);
        else {
            Scope<ConfigReader> layers{ config_reader->get_child(u8"Physics Layers") };
            if(layers) layers->for_each
            ([&config](String name, Scope<ConfigReader> layer)
            {
                int idx{ std::stoi(name.raw()) };
                if(idx >= 16) return;
                config.physics_layers.set_name(static_cast<uint8>(idx),
                    layer->get_at(u8"Name", String(PhysicsLayers::default_name)));
                config.physics_layers.set_mask(static_cast<uint8>(idx),
                    layer->get_at(u8"Mask", PhysicsLayers::default_mask));
            });
            uint8 showed_layers{ static_cast<uint8>
                (config_reader->get_at(u8"Showed Layers", 1ui32)) };
            config.physics_layers.set_showed_layer_count(showed_layers);
            config.anti_aliasing = static_cast<AntiAliasing>(config_reader->
                get_at(u8"Anti-Aliasing", static_cast<int>(AntiAliasing::MSAAx4)));
        }

        assets_manager_ = create_scope<AssetsManager>(get_assets_dir());
        script_registry_ = create_scope<ScriptRegistry>();
        script_dylib_loader_ = ScriptDylibLoader::create
            (project_dir_ / u8"bin" / RKE_CONFIG_NAME, project_config_.name);

        CORE_INFO(u8"Project: Project '{}' loaded.", rkproj_path_);
        scripts_hot_reloading();
    }

    Project::~Project()
    {
        script_registry_->clear();
        script_dylib_loader_.reset();
    }

    bool Project::save()
    {
        if(rkproj_path_.empty()) {
            CORE_ERROR(u8"Project: Project has no save path!");
            return false;
        }
        if(!rkproj_path_.exists()) {
            CORE_ERROR(u8"Project: Project file '{}' does not exist! "
                u8"Maybe deleted.", rkproj_path_);
            return false;
        }

        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map();

        writer->write(u8"Project", project_config_.name);

        writer->begin_map(u8"Config");
        writer->begin_map(u8"Physics Layers");
        auto& layers{ project_config_.physics_layers };
        for(uint8 i{}; i < PhysicsLayers::max_layers; i++)
        {
            String count_str{ String::format(u8"{}", static_cast<uint32>(i)) };
            writer->begin_map(StringView(count_str));
            writer->write(u8"Name", layers.get_name(i));
            writer->write(u8"Mask", layers.get_mask(i));
            writer->end_map();
        }
        writer->end_map(); // Physics Layers
        writer->write(u8"Showed Layers", static_cast<uint32>(layers.get_showed_layer_count()));
        writer->write(u8"Anti-Aliasing", static_cast<int>(project_config_.anti_aliasing));
        writer->end_map(); // Config

        writer->end_map();
        writer->push_to_file(rkproj_path_);
        CORE_INFO(u8"Project: Project '{}' saved.", rkproj_path_);
        return true;
    }

    bool Project::scripts_hot_reloading()
    {
        script_registry_->clear();
        if(script_dylib_loader_->load_dylib())
        {
            auto registar{ script_dylib_loader_->get_register_scripts_func() };
            if(!registar(script_registry_.get()))
            {
                for(auto& [_, scene] : scene_map_)
                    scene->on_script_dylib_hot_reloading(*script_registry_);
                CORE_ERROR(u8"Project: Failed to register scripts!");
                return false;
            }
            for(auto& [_, scene] : scene_map_)
                scene->on_script_dylib_hot_reloading(*script_registry_);
            CORE_INFO(u8"Project: Scripts Registered.");
            return true;
        }
        for(auto& [_, scene] : scene_map_)
            scene->on_script_dylib_hot_reloading(*script_registry_);
        CORE_ERROR(u8"Project: Failed to load dylib!");
        return false;
    }

    bool Project::create_scene(const String& name)
    {
        Path new_scene_path{ get_scenes_dir() / (name + u8".rkscene")};
        if(new_scene_path.exists()) {
            CORE_WARN(u8"Project: Scene '{}' already exists! "
                u8"Please choose an another name.", new_scene_path);
            return false;
        }
        if(name.empty()) {
            CORE_WARN(u8"Project: Scene name empty!");
            return false;
        }
        SceneSerializer scene_creator_{}; // TO MODIFY
        Scope<Scene> empty_scene{ create_scope<Scene>(this, name) };
        scene_creator_.serialize(*empty_scene, new_scene_path);
        return true;
    }

    Scene* Project::load_scene(const String& name, SceneSerializer& scene_serializer)
    {
        if(scene_map_.contains(name)) return scene_map_.at(name).get();

        Path scene_path{ get_scenes_dir() / (name + u8".rkscene") };
        if(!scene_path.exists()) {
            CORE_ERROR(u8"Project: Scene '{}' not found!", scene_path);
            return nullptr;
        }
        Scope<Scene> scene{ create_scope<Scene>(this, name) };
        if(scene_serializer.deserialize(*scene, scene_path))
        {
            CORE_INFO(u8"Project: Scene '{}' loaded.", scene_path);
            Scene* handle{ scene.get() };
            scene_map_.emplace(name, std::move(scene));
            return handle;
        }
        CORE_ERROR(u8"Project: Could not load scene '{}'!", scene_path);
        return nullptr;
    }

    void Project::save_scene(const Scene& scene, SceneSerializer& scene_serializer)
    {
        if(scene.get_owner() != this) {
            CORE_ERROR(u8"Project: Scene '{}' "
                u8"doesn't belong to this project!", scene.get_name());
            return;
        }
        scene_serializer.serialize(scene, scene.get_path());
    }

    void Project::save_scene(const String& name, SceneSerializer& scene_serializer)
    {
        Path scene_path{ get_scenes_dir() / (name + u8".rkscene") };
        if(!scene_path.exists()) {
            CORE_ERROR(u8"Project: Path '{}' not found!", scene_path);
            return;
        }
        if(!scene_map_.contains(name)) {
            CORE_ERROR(u8"Project: Scene '{}' not loaded!", name);
            return;
        }
        scene_serializer.serialize(*(scene_map_.at(name)), scene_path);
    }

    void Project::remove_scene(const String& name) { scene_map_.erase(name); }
    
// static
    void Project::init_templates(const Path& templates_path)
    {
        if(s_cmake_lists_txt.empty()) { s_cmake_lists_txt =
            file::read_file_string(templates_path / u8"CMakeLists.txt.txt"); }
        if(s_cmake_presets_json.empty()) { s_cmake_presets_json =
            file::read_file_string(templates_path / u8"CMakePresets.json.txt"); }
        if(s_myscript_cpp.empty()) { s_myscript_cpp =
            file::read_file_string(templates_path / u8"MyScript.cpp.txt"); }
        if(s_editorconfig.empty()) { s_editorconfig =
            file::read_file_string(templates_path / u8".editorconfig.txt"); }
    }

    bool Project::create_files(const Path& rkproj_path)
    {
        if(rkproj_path.extension() != u8".rkproj") {
            CORE_ERROR(u8"Project: Invalid project file path!");
            return false;
        }

        Path project_dir{ rkproj_path.parent_path() };
        String project_name{ rkproj_path.stem().string() }; // stem() for filename with no extension

        if(!project_dir.exists()) fs::create_directories(project_dir);
        else if(!project_dir.empty()) {
            CORE_ERROR(u8"Project: Directory '{}' not empty!", project_dir);
            return false;
        }

        Path assets_dir{ project_dir / u8"assets" };
        Path scenes_dir{ assets_dir  / u8"scenes" };
        Path src_dir   { project_dir / u8"src"    };
        fs::create_directory(assets_dir );
        fs::create_directory(scenes_dir );
        fs::create_directory(src_dir    );

        String cmake_content{ s_cmake_lists_txt };
        cmake_content.replace_search_by(u8"%{ProjectName}", project_name);
        file::write_file_string(project_dir / u8"CMakeLists.txt", cmake_content);

        file::write_file_string(project_dir / u8"CMakePresets.json", s_cmake_presets_json);
        file::write_file_string(project_dir / u8".editorconfig", s_editorconfig);
        file::write_file_string(src_dir     / u8"MyScript.cpp" , s_myscript_cpp);

        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map();

        writer->write(u8"Project", project_name);

        writer->begin_map(u8"Config");
        writer->begin_map(u8"Physics Layers");
        for(uint8 i{}; i < PhysicsLayers::max_layers; i++)
        {
            String count_str{ String::format(u8"{}", static_cast<uint32>(i)) };
            writer->begin_map(StringView(count_str));
            writer->write(u8"Name", String(PhysicsLayers::default_name));
            writer->write(u8"Mask", PhysicsLayers::default_mask);
            writer->end_map();
        }
        writer->end_map(); // Physics Layers
        writer->write(u8"Showed Layers", 1);
        writer->write(u8"Anti-Aliasing", static_cast<int>(AntiAliasing::MSAAx4));
        writer->end_map(); // Config

        writer->end_map();
        writer->push_to_file(rkproj_path);
        CORE_INFO(u8"Project: Created new project '{}.rkproj' at '{}'.",
            project_name, project_dir);
        return true;
    }
}
