module;

#include <utility>
#include <memory>
#include <filesystem>
#include "rke_macros.h"

export module Project;

import Scene;
import String;
import Path;
import HeapManager;
import PhysicsLayers;

export namespace rke
{
    class RKE_API Project
    {
    public:
        friend struct std::default_delete<Project>;

        struct Config // serialized in .rkproj file
        {
            String name{ u8"Untitled" };
            String start_scene{};

            PhysicsLayers physics_layers{};
            int anti_aliasing_opt{ 2 };
        };

        Project(const Project&) = delete; // Only one active project(for now)
        Project(Project&& ____) = delete;

        bool scripts_hot_reloading();
        bool save(); // both project and active scene
        const Path& get_project_dir() const { return project_dir_; }
        Path get_assets_dir() const;
        Path get_scenes_dir() const;
        Path get_active_scene_path() const;

        const Config& get_config() const { return project_config_; }
        Config& get_config_mut() { return project_config_; }
        Ref<Scene> get_active_scene() { return active_scene_; } // can be null
        Ref<Scene> load_scene(const Path& filepath);
        void clear_active_scene();

        static Project* get_active_project();

        static void init_file_templates(const Path& templates_path);
        static bool create(const Path& path);
        static Project* load_to_active(const Path& path);
        static bool save_active();
        static void release_active();

        void set_name(String name) { project_config_.name = std::move(name); }
        const String& get_name() const { return project_config_.name; }
    private:
        Project();
        ~Project();
    private:
        Config project_config_{};
        Ref<Scene> active_scene_{};
        Path project_dir_{}; // to project folder
        Path rkproj_path_{}; // to .rkproj file
    };
}
