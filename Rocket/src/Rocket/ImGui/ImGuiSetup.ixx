module;

#include <imgui.h>
#include <imgui_internal.h>
#include "rke_macros.h"

export module ImGuiSetup;

import NativeWindow;

export namespace rke::imgui
{
    void init(NativeWindow context);
    void shutdown();

    void begin_render();
    void end_render();

    RKE_API void disable_bind_sampler();
}

namespace rke::imgui::internal
{
    void init_window(NativeWindow context);
    void init_graphics();

    void shutdown_window();
    void shutdown_graphics();

    void begin_render_window();
    void begin_render_graphics();

    void end_render_window();
    void end_render_graphics();
}
