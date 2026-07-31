module;
module Project;

import Log;
import SceneSerializer;
import FileUtils;
import ScriptLoader;
import ScriptRegistry;
import AssetsManager;
import ConfigProxy;
import ProjectEvent;
import Application;

namespace rke
{
    static String s_cmake_lists_txt{};
    static String s_cmake_presets_json{};
    static String s_myscript_ixx{};
    static String s_editorconfig{};

    Project::~Project()
    {
        ScriptLoader::unload_all_dylibs();
        ScriptLoader::delete_temp_files(project_dir_ / u8"bin" / RKE_CONFIG_NAME);
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
        return true;
    }

    bool Project::scripts_hot_reloading()
    {
    // TO MODIFY
        Path dylib_dir{ project_dir_ / u8"bin" / RKE_CONFIG_NAME };
        return ScriptLoader::load_dylib(dylib_dir, project_config_.name);
    // TO MODIFY
    }

    void Project::init_templates(const Path& templates_path)
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
        file::write_file_string(src_dir     / u8"MyScript.ixx" , s_myscript_ixx);

        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map();

        writer->write(u8"Project", project_name);

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

    Scope<Project> Project::load_from(const Path& path)
    {
        Scope<Project> project(new Project());
        // can't use create_scope<...> here, but
        // which does exactly the same thing anyway.
        project->rkproj_path_ = path;
        project->project_dir_ = path.parent_path();

        Scope<ConfigReader> reader{ ConfigReader::create(path) };
        auto& config{ project->project_config_ };
        config.name = reader->get_at(u8"Project", String{});
        if(config.name.empty()) {
            CORE_ERROR(u8"Project: Invalid project file format in '{}'!", path);
            return nullptr;
        }
        
        Scope<ConfigReader> config_reader{ reader->get_child(u8"Config") };
        if(!config_reader)
            CORE_ERROR(u8"Project: 'Config' not found in '{}'!", path);
        else {
            Scope<ConfigReader> layers{ config_reader->get_child(u8"Physics Layers") };
            if(layers) layers->for_each
            ([&config](String name, Scope<ConfigReader> layer)
            {
                int idx{ std::stoi(name.raw()) };
                if(idx >= 16) return;
                config.physics_layers.set_name(static_cast<uint8>(idx),
                    layer->get_at(u8"Name", PhysicsLayers::get_default_name()));
                config.physics_layers.set_mask(static_cast<uint8>(idx),
                    layer->get_at(u8"Mask", PhysicsLayers::get_default_mask()));
            });
            uint8 showed_layers{ static_cast<uint8>
                (config_reader->get_at(u8"Showed Layers", 1ui32)) };
            config.physics_layers.set_showed_layer_count(showed_layers);
            config.anti_aliasing_opt = config_reader->
                get_at(u8"Anti-Aliasing Option", config.anti_aliasing_opt);
        }
        CORE_INFO(u8"Project: Project '{}' loaded.", project->rkproj_path_);
        ProjectLoadedEvent event{ u8"main" };
        app().send_event(event);
        project->scripts_hot_reloading();
        return project;
    }
}
