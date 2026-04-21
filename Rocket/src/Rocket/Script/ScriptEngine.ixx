module;

#include "rke_macros.h"

export module ScriptEngine;

import Types;
import Scene;
import Event;
import String;

export namespace rke
{
    class ScriptEngine
    {
    public:
        static void on_runtime_start(Scene* scene);
        static void on_runtime_stop ();

        static void on_update(float dt);
        static void on_mouse_scrolled(MouseScrolledEvent& e);

        static RKE_API void update_script(Entity entity);

        static void create_script(uint32 handle, const String& name);
        static void destroy_script(uint32 handle);
    };
}
