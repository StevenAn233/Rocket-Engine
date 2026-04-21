module;
module Project;

import Log;
import SceneSerializer;
import FileUtils;
import ScriptLoader;
import ScriptRegistry;
import AssetsManager;
import ConfigProxy;

namespace {
    using namespace rke;

    static String s_cmake_lists_txt{};
    static String s_cmake_presets_json{};
    static String s_myscript_ixx{};
    static String s_editorconfig{};

    static Scope<Project> s_active_project{};
}

namespace rke
{
    Project::Project() {}
    Project::~Project() { ScriptLoader::unload_all(); }

    bool Project::scripts_hot_reloading()
    {
    // Detach current scene first!
        clear_active_scene();

        bool succeeded{ true };
    // Load Script dll: Must be ahead of scene loading!
        Path dynamic_lib_path{ project_dir_ / u8"bin" / RKE_CONFIG_NAME };
        // Clean up temp loaded dlls
        if(dynamic_lib_path.exists())
            for(const auto& entry : fs::directory_iterator(dynamic_lib_path))
            {
                String filename{ Path(entry.path().filename()).string() };
                if(filename.find(u8"loaded") != String::npos)
                {
                    std::error_code ec{};
                    fs::remove(entry.path(), ec);
                    if(!ec) CORE_INFO(u8"Project: Deleted temporary '{}'.", filename);
                }
            }
        // previous dll still bound, so can't be deleted
        // (doesn't really matter, only one dll bound)

    #ifdef RKE_PLATFORM_WINDOWS
        Path script_dll_path{ dynamic_lib_path / (project_config_.name + u8".dll") };
    #else
        static_assert(false, u8"Project: Other OS has not been supported yet!");
    #endif

        if(script_dll_path.exists()) {
            if(!ScriptLoader::load_script_dll(script_dll_path)) {
                CORE_ERROR(u8"Project: Could not load script DLL at '{}'!", script_dll_path);
                succeeded = false;
            }
        } else {
            ScriptRegistry::clear();
            ScriptLoader::unload_all();
            if(dynamic_lib_path.exists())
                CORE_ERROR(u8"Project: Script DLL '{}' doesn't exist!", script_dll_path);
            succeeded = false;
        }

    // Reload Scene(or load new scene)
        Path current_scene_path{ get_assets_dir() / u8"scenes" / project_config_.start_scene };
        bool is_rkscene{ current_scene_path.filename().string().ends_with(u8".rkscene") };
        if(current_scene_path.exists() && is_rkscene) load_scene(current_scene_path);
        return succeeded;
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
        writer->write(u8"Start Scene", project_config_.start_scene);

        writer->begin_map(u8"Config");
        writer->begin_map(u8"Physics Layers");
        auto& layers{ project_config_.physics_layers };
        for(uint8 i{}; i < PhysicsLayers::get_layer_count(); i++)
        {
            String count_str{ String::format(u8"{}", static_cast<uint32>(i)) };
            writer->begin_map(StringView(count_str));
            writer->write(u8"Name", layers.get_name(i));
            writer->write(u8"Mask", layers.get_mask(i));
            writer->end_map();
        }
        writer->end_map(); // Physics Layers
        writer->write(u8"Showed Layers", static_cast<uint32>
            (layers.get_showed_layer_count()));
        writer->write(u8"Anti-Aliasing Option", project_config_.anti_aliasing_opt);
        writer->end_map(); // Config

        writer->end_map();
        writer->push_to_file(rkproj_path_);
        CORE_INFO(u8"Project: Project '{}' saved.", rkproj_path_);

        Path scene_path{ get_assets_dir() / u8"scenes" / project_config_.start_scene };
        String scene_path_str{ scene_path.string() };
        if(scene_path.exists() && scene_path_str.ends_with(u8".rkscene"))
        {
            SceneSerializer serializer(active_scene_);
            
            if(serializer.serialize(scene_path_str)) {
                CORE_INFO(u8"Project: Active scene '{}' saved.", scene_path_str);
                return true;
            } else {
                CORE_ERROR(u8"Project: Failed to save active scene '{}'!", scene_path_str);
                return false;
            }
        }
        else if(!project_config_.start_scene.empty())
        {
            CORE_ERROR(u8"Project: Scene '{}' doesn't exist "
                u8"or is not a valid rkscene!", scene_path_str);
            scene_path.clear();
        }
        return false;
    }

    Ref<Scene> Project::load_scene(const Path& filepath)
    {
        CORE_ASSERT(filepath.parent_path() == get_scenes_dir(),
            u8"Project: Scene dir '{}' doesn't match with '{}'!",
            filepath.parent_path(), get_assets_dir());

        String filepath_str{ filepath.string() };
        if(filepath.exists() && filepath_str.ends_with(u8".rkscene"))
            project_config_.start_scene = filepath.filename().string();
        else {
            CORE_WARN(u8"Project: Scene '{}' doesn't exist or is not a valid scene!", filepath);
            return nullptr;
        }

        Ref<Scene> scene{ create_ref<Scene>() };
        SceneSerializer serializer(scene);

        if(serializer.deserialize(filepath_str))
        {
            clear_active_scene();
            active_scene_ = scene;
            active_scene_->on_attach();
            CORE_INFO(u8"Project: Scene '{}' loaded.", filepath);
            return scene;
        }
        CORE_ERROR(u8"Project: Could not load scene '{}'!", filepath);
        return nullptr;
    }

    void Project::clear_active_scene()
    {
        if(active_scene_) {
            CORE_ASSERT(!active_scene_->in_runtime(),
                u8"Project: Can't detach or clear runtime-scene!");
            active_scene_->on_detach();
            CORE_INFO(u8"Project: Active scene cleared.");
        }
        active_scene_ = nullptr;
    }

    Path Project::get_assets_dir() const { return project_dir_ / u8"assets"; }
    Path Project::get_scenes_dir() const { return project_dir_ / u8"assets" / u8"scenes"; }
    Path Project::get_active_scene_path() const
        { return get_scenes_dir() / project_config_.start_scene; }

    Project* Project::get_active_project() { return s_active_project.get(); }

    void Project::init_file_templates(const Path& templates_path)
    {
        if(s_cmake_lists_txt.empty()) { s_cmake_lists_txt =
            file::read_file_string(templates_path / u8"CMakeLists.txt.txt"); }
        if(s_cmake_presets_json.empty()) { s_cmake_presets_json =
            file::read_file_string(templates_path / u8"CMakePresets.json.txt"); }
        if(s_myscript_ixx.empty()) { s_myscript_ixx =
            file::read_file_string(templates_path / u8"MyScript.ixx.txt"); }
        if(s_editorconfig.empty()) { s_editorconfig =
            file::read_file_string(templates_path / u8".editorconfig.txt"); }
    }

    bool Project::create(const Path& rkproj_path)
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
        file::write_file_string(src_dir     / u8"MyScript.ixx" , s_myscript_ixx);

        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map();

        writer->write(u8"Project", project_name);
        writer->write(u8"Start Scene", String{});

        writer->begin_map(u8"Config");
        writer->begin_map(u8"Physics Layers");
        for(uint8 i{}; i < PhysicsLayers::get_layer_count(); i++)
        {
            String count_str{ String::format(u8"{}", static_cast<uint32>(i)) };
            writer->begin_map(StringView(count_str));
            writer->write(u8"Name", PhysicsLayers::get_default_name());
            writer->write(u8"Mask", PhysicsLayers::get_default_mask());
            writer->end_map();
        }
        writer->end_map(); // Physics Layers
        writer->write(u8"Showed Layers", 1);
        writer->write(u8"Anti-Aliasing Option", 2);
        writer->end_map(); // Config

        writer->end_map();
        writer->push_to_file(rkproj_path);
        CORE_INFO(u8"Project: Created new project '{}.rkproj' at '{}'.",
            project_name, project_dir);
        return true;
    }

    Project* Project::load_to_active(const Path& path)
    {
        Scope<Project> project(new Project{});
        // can't use create_scope<...> here, but
        // which does exactly the same thing anyway.
        project->rkproj_path_ = path;
        project->project_dir_ = path.parent_path();

        Scope<ConfigReader> reader{ ConfigReader::create(path) };
        auto& config{ project->project_config_ };
        config.name = reader->get_at(u8"Project", String{});
        config.start_scene = reader->get_at(u8"Start Scene", String{});
        if(config.name.empty()) {
            CORE_ERROR(u8"Project: Invalid project file format in '{}'!", path);
            return s_active_project.get();
        }
        if(!config.start_scene.empty()) {
            Path start_scene_path{ project->get_scenes_dir() / config.start_scene };
            if(!start_scene_path.exists()) {
                CORE_ERROR(u8"Project: Start scene '{}' not found!", start_scene_path);
                config.start_scene.clear();
            }
        }
        
        Scope<ConfigReader> config_reader{ reader->get_child(u8"Config") };
        if(!config_reader)
            CORE_ERROR(u8"Project: 'Config' not found in '{}'!", path);
        else {
            Scope<ConfigReader> layers{ config_reader->get_child(u8"Physics Layers") };
            if(layers) {
                layers->for_each_map([&config](String name, Scope<ConfigReader> layer)
                {
                    int idx{ std::stoi(name.raw()) };
                    if(idx >= 16) return;
                    config.physics_layers.set_name(static_cast<uint8>(idx),
                        layer->get_at(u8"Name", PhysicsLayers::get_default_name()));
                    config.physics_layers.set_mask(static_cast<uint8>(idx),
                        layer->get_at(u8"Mask", PhysicsLayers::get_default_mask()));
                });
            }
            uint8 showed_layers{ static_cast<uint8>
                (config_reader->get_at(u8"Showed Layers", 1ui32)) };
            config.physics_layers.set_showed_layer_count(showed_layers);
            config.anti_aliasing_opt = config_reader->
                get_at(u8"Anti-Aliasing Option", config.anti_aliasing_opt);
        }
        if(s_active_project) s_active_project->clear_active_scene();
        CORE_INFO(u8"Project: Project '{}' loaded.", project->rkproj_path_);
        s_active_project = std::move(project);

        s_active_project->scripts_hot_reloading();
        return s_active_project.get();
    }

    bool Project::save_active()
    {
        if(!s_active_project) {
            CORE_WARN(u8"Project: No active project!");
            return false;
        }
        return s_active_project->save();
    }

    void Project::release_active() { s_active_project.release(); }
}
