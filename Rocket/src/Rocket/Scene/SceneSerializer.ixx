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
        using SerializeHook = std::function<void(const Scene&, ConfigWriter&)>;
        using DeserializeHook = std::function<void(Scene&, const ConfigReader&)>;

        static bool serialize(const Scene& scene, const Path& filepath);
        static bool deserialize(Scene& scene, const Path& filepath);

        static void set_serialize_hook(SerializeHook hook);
        static void set_deserialize_hook(DeserializeHook hook);
    };
}
