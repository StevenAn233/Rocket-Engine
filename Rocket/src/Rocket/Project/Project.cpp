module;
module Project;

import Log;
import FileUtils;
import ProjectEvent;
import Application;

namespace {
    using namespace rke;
    String read_template(StringView file_name)
    {
        return file::read_file_string
            (file::assets_dir() / u8"proj-templates" / file_name);
    }
}

namespace rke
{
    Project::Project(const Path& rkproj_path)
        : rkproj_path_(rkproj_path), project_dir_(rkproj_path.parent_path())
    {
        Scope<ConfigReader> reader{ ConfigReader::create(rkproj_path) };
        if(!reader) {
            CORE_ERROR(u8"Project: Project path '{}' invalid!",
                rkproj_path);
            return;
        }
        name_ = reader->get_at(u8"Project", String{});
        if(name_.empty()) {
            CORE_ERROR(u8"Project: Invalid project file format in '{}'!",
                rkproj_path);
            return;
        }
        
        assets_manager_ = create_scope<AssetsManager>(get_assets_dir());
        dylib_loader_ = create_scope<RkDylibLoader>
            (project_dir_ / u8"bin" / RKE_CONFIG_NAME, name_);
        scripts_hot_reloading();
        CORE_INFO(u8"Project: Project '{}' loaded.", rkproj_path_);

        auto& config{ project_config_ };
        Scope<ConfigReader> config_reader{ reader->get_child(u8"Config") };
        if(config_reader) {
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
            config.samples = config_reader->get_at(u8"Samples", 1u);
        }
        if(fresh_dylib_) post_processor_.deserialize_from(*reader, *fresh_dylib_);
    }

    Project::~Project()
    {
        scene_map_.clear();
        post_processor_.clear();
        fresh_dylib_.reset();
        dylib_loader_.reset();
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

        writer->write(u8"Project", name_);

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
        writer->write(u8"Samples", project_config_.samples);

        writer->end_map(); // Config

        if(fresh_dylib_) post_processor_.serialize_to(*writer, *fresh_dylib_);

        writer->end_map();
        writer->push_to_file(rkproj_path_);
        CORE_INFO(u8"Project: Project '{}' saved.", rkproj_path_);
        return true;
    }

    void Project::scripts_hot_reloading()
    {
        Scope<RkDylib> dylib{ dylib_loader_->load_fresh() };
        if(!dylib) {
            CORE_ERROR(u8"Project: Failed to load dylib!");
            return;
        }
        dylib->owner_ = this;

        Scope<RkDylib> old_dylib{ std::move(fresh_dylib_) };
        fresh_dylib_ = std::move(dylib);

        String snapshot{};
        if(old_dylib) {
            Scope<ConfigWriter> writer{ ConfigWriter::create() };
            CORE_ASSERT(writer, u8"Project: Failed to create config writer!");
            writer->begin_map();
            post_processor_.serialize_to(*writer, *old_dylib);
            writer->end_map();
            snapshot = writer->dump();
        }
        if(!snapshot.empty())
        {
            Scope<ConfigReader> reader{ ConfigReader::create_from_string(snapshot) };
            CORE_ASSERT(reader, u8"Project: Failed to create config reader!");
            post_processor_.deserialize_from(*reader, *fresh_dylib_);
        }

        if(old_dylib) {
            for(auto& [_, scene] : scene_map_)
                scene->on_script_dylib_hot_reloading(*old_dylib, *fresh_dylib_);
        }
    }

    uint8 Project::get_samples_shift() const
    {
        uint8 shift{};
        uint32 samples{ get_config().samples }; // copy
        while(samples = samples >> 1) ++shift;
        return shift;
    }

    void Project::set_samples(uint8 shift)
    {
        constexpr uint8 max_shift{ 5 }; // to modify
        if(shift > max_shift) {
            CORE_WARN(u8"Project: Samples too much!");
            return;
        }
        project_config_.samples = (1u << shift);
        ProjectSamplesSetEvent event{ u8"main", this, project_config_.samples };
        app().send_event(event);
    }

    bool Project::create_scene(const String& name)
    {
        Path new_scene_path{ get_scenes_dir() / (name + u8".rkscene") };
        if(new_scene_path.exists()) {
            CORE_WARN(u8"Project: Scene '{}' already exists! "
                u8"Please choose an another name.", new_scene_path);
            return false;
        }
        if(name.empty()) {
            CORE_WARN(u8"Project: Scene name empty!");
            return false;
        }
        SceneSerializer scene_creator_{}; // may modify
        Scope<Scene> empty_scene{ new Scene(this, name) };
        scene_creator_.serialize(*empty_scene, new_scene_path);
        return true;
    }

    Scene* Project::load_scene(const String& name, SceneSerializer& scene_serializer)
    {
        if(name.empty()) return nullptr;
        if(scene_map_.contains(name)) return scene_map_.at(name).get();

        Path scene_path{ get_scenes_dir() / (name + u8".rkscene") };
        if(!scene_path.exists()) {
            CORE_ERROR(u8"Project: Scene '{}' not found!", scene_path);
            return nullptr;
        }
        Scope<Scene> scene{ new Scene(this, name) };
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

    const GTexture2D* Project::post_process(const GTexture2D* source)
        { return post_processor_.process(source); }

    // not responsible for scene viewport set; may modify
    void Project::on_viewport_resized(uint32 w, uint32 h)
        { post_processor_.on_viewport_resized(w, h); }
    
// static
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

        Path assets_dir { project_dir / u8"assets"  };
        Path scenes_dir { assets_dir  / u8"scenes"  };
        Path shaders_dir{ assets_dir  / u8"shaders" };
        Path src_dir{ project_dir / u8"src" };
        fs::create_directory(assets_dir);
        fs::create_directory(scenes_dir);
        fs::create_directories(shaders_dir / u8"src");
        fs::create_directories(shaders_dir / u8"include");
        fs::create_directory(src_dir);

        String cmake_content{ read_template(u8"CMakeLists.txt.txt") };
        cmake_content.replace_search_by(u8"%{ProjectName}", project_name);
        file::write_file_string(project_dir / u8"CMakeLists.txt", cmake_content);

        file::write_file_string(project_dir / u8"CMakePresets.json",
            read_template(u8"CMakePresets.json.txt"));
        file::write_file_string(project_dir / u8"MyScript.cpp",
            read_template(u8"MyScript.cpp.txt"));
        file::write_file_string(src_dir / u8".editorconfig",
            read_template(u8".editorconfig.txt"));

        file::write_file_string(src_dir / u8"VignetteEffect.cpp",
            read_template(u8"VignetteEffect.cpp.txt"));
        file::write_file_string(shaders_dir / u8"vignette.rkshdr",
            read_template(u8"vignette.rkshdr.txt"));
        file::write_file_string(shaders_dir / u8"src" / u8"vignette.frag",
            read_template(u8"vignette.frag.txt"));

        // may modify
        file::write_file_string(shaders_dir / u8"src" / u8"fullscreen.vert",
            file::read_file_string(file::assets_dir()
                / u8"shaders" / u8"src" / u8"fullscreen.vert"));
        file::write_file_string(shaders_dir / u8"include" / u8"binding_points.glsl",
            file::read_file_string(file::assets_dir()
                / u8"shaders" / u8"include" / u8"binding_points.glsl"));

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
        writer->write(u8"Samples", 1);
        writer->end_map(); // Config

        writer->end_map();
        writer->push_to_file(rkproj_path);
        CORE_INFO(u8"Project: Created new project '{}.rkproj' at '{}'.",
            project_name, project_dir);
        return true;
    }
}
