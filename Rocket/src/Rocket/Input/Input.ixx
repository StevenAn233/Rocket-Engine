// Input system is for continuous operation(e.g. moving)
// Event system if for one-key-triggered operation(e.g. esc to open menu)
module;

#include <utility>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Input;

import Keys;
import MouseButtons;

export namespace rke
{
    class RKE_API Input
    {
    public:
        static void transition_input_state(bool block_mouse, bool block_keyboard);

        static constexpr bool is_key_pressed(Key keycode);
        static constexpr bool is_mouse_button_pressed(Mouse button);

        static glm::vec2 get_mouse_pos_in_window();
        static float get_mouse_x_in_window();
        static float get_mouse_y_in_window();
    private:
        static bool s_block_mouse_;
        static bool s_block_keyboard_;
    };
}
