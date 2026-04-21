module;

#include <functional>
#include "rke_macros.h"

export module SceneSerializer;

import String;
import Path;
import HeapManager;
import Scene;
import ConfigProxy;

export namespace rke
{
    class RKE_API SceneSerializer
    {
    public:
        using SerializeHook = std::function<void(SceneSerializer*, ConfigWriter*)>;
        using DeserializeHook = std::function
            <void(SceneSerializer*, Scene*, const ConfigReader*)>;

        SceneSerializer(Ref<Scene> scene);
        SceneSerializer(const SceneSerializer&) = delete;
        SceneSerializer(SceneSerializer&& ____) = delete;

        bool serialize  (const Path& filepath);
        bool deserialize(const Path& filepath);
        bool is_to_serialize(Scene* scene) { return scene_.get() == scene; }

        static void set_serialize_hook(SerializeHook hook);
        static void set_deserialize_hook(DeserializeHook hook);
    private:
        Ref<Scene> scene_;
    };
}
