module;
export module Gizmo;

import rke;
import EditorCamera;

export namespace rke::gizmo
{
    enum class Mode
    {
        Translate,
        Rotate,
        Scale
    };

    void on_render(Scene& scene, Mode mode, const EditorCamera& cam, bool mouse_blocked);

    bool is_over();
    bool is_using();
}
