module;

#include <vector>
#include <memory>
#include <utility>
#include <filesystem>
#include <unordered_map>
#include "rke_macros.h"

export module Project;

import Path;
import Types;
import String;
import HeapManager;
import Scene;
import SceneSerializer;
import PhysicsLayers;
import AssetsManager;
import RkDylib;
import RkDylibLoader;
import PostProcessor;
import ConfigProxy;
import GTexture;

export namespace rke
{
    class RKE_API Project
    {
    public:
        friend class Application;
        friend class ProjectSettingPanel;

        Project(const Path& rkproj_path);
        ~Project();

        struct Config // serialized in .rkproj file
        {
            PhysicsLayers physics_layers{};
            uint32 samples{};
        };

        Project(const Project&) = delete; // Only one active project
        Project(Project&&) = delete;

        bool save();
        void scripts_hot_reloading();

        inline const String& get_name() const { return name_; }
        inline const Config& get_config() const { return project_config_; }
        inline const Path& get_project_dir() const { return project_dir_; }
        inline const Path& get_rkproj_path() const { return rkproj_path_; }
        inline Path get_assets_dir() const { return project_dir_ / u8"assets"; }
        inline Path get_scenes_dir() const { return project_dir_ / u8"assets" / u8"scenes"; }

        inline PhysicsLayers& get_physics_layers() { return project_config_.physics_layers; }
        inline const PhysicsLayers& get_physics_layers() const { return project_config_.physics_layers; }

        inline void set_name(String name) { name_ = std::move(name); }
        uint8 get_samples_shift() const;
        void set_samples(uint8 shift);

        inline const AssetsManager& get_assets_manager() const { return *assets_manager_; }
        inline AssetsManager& get_assets_manager_mut() { return *assets_manager_; }
        inline const RkDylib* get_dylib() const { return fresh_dylib_.get(); }

        bool create_scene(const String& name);
        Scene* load_scene(const String& name, SceneSerializer& scene_serializer);
        void save_scene(const Scene& scene, SceneSerializer& scene_serializer);
        void save_scene(const String& name, SceneSerializer& scene_serializer);
        void remove_scene(const String& name);

        const GTexture2D* post_process(const GTexture2D* source);
        void on_viewport_resized(uint32 w, uint32 h);

        static bool create_files(const Path& path);
    private:
        Path project_dir_; // to project folder
        Path rkproj_path_; // to .rkproj file

        String name_{};
        Config project_config_{};

        std::unordered_map<String, Scope<Scene>> scene_map_{};
        // SceneManager scene_manager_;
        Scope<AssetsManager> assets_manager_{};

        Scope<RkDylibLoader> dylib_loader_{};
        Scope<RkDylib> fresh_dylib_{};

        PostProcessor post_processor_{};
    };
}
