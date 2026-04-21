module;
export module Gizmo;

import rke;
import EditorCamera;

export namespace rke
{
    class Gizmo
    {
    public:
        enum class Mode
        {
            Translate,
            Rotate,
            Scale
        };

        static void on_render(Entity selected_entity, Mode mode,
                              const EditorCamera& cam, bool mouse_blocked);

        static bool is_over ();
        static bool is_using();
    };
}
