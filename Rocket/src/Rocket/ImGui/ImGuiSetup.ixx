export module ImGuiSetup;

import NativeWindow;

export namespace rke::imgui
{
    void init(NativeWindow context);
    void shutdown();

    void begin_render();
    void end_render();
}
