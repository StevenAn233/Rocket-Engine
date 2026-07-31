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
        friend class Application;
        friend struct std::default_delete<Project>;

        struct Config // serialized in .rkproj file
        {
            String name{ u8"Untitled" };
            PhysicsLayers physics_layers{};
            int anti_aliasing_opt{ 2 };
        };

        Project(const Project&) = delete; // Only one active project
        Project(Project&&) = delete;

        bool save();
        bool scripts_hot_reloading();

        inline void set_name(String name) { project_config_.name = std::move(name); }
        inline const String& get_name() const { return project_config_.name; }

        inline const Path& get_project_dir() const { return project_dir_; }
        inline Path get_assets_dir() const { return project_dir_ / u8"assets"; }
        inline Path get_scenes_dir() const { return project_dir_ / u8"assets" / u8"scenes"; }

        inline const Config& get_config() const { return project_config_; }
        inline Config& get_config_mut() { return project_config_; }

        static void init_templates(const Path& templates_path);
        static bool create_files(const Path& path);
        static Scope<Project> load_from(const Path& path);
    private:
        Project() = default;
        ~Project();
    private:
        Config project_config_{};
        Path project_dir_{}; // to project folder
        Path rkproj_path_{}; // to .rkproj file
        // SceneManager manager_;
    };
}
