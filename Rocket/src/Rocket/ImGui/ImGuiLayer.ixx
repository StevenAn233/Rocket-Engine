module;

#include <utility>
#include "rke_macros.h"

export module ImGuiLayer;

import NativeWindow;

export namespace rke::imgui
{
    RKE_API void init(NativeWindow context);
    RKE_API void shutdown();

    RKE_API void begin_render();
    RKE_API void end_render();
}
