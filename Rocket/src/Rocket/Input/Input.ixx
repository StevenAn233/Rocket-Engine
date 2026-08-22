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
        void transition_input_state(bool block_mouse, bool block_keyboard);

        bool is_key_pressed(Key keycode) const;
        bool is_mouse_button_pressed(Mouse button) const;

        glm::vec2 get_mouse_pos_in_window() const;
        float get_mouse_x_in_window() const;
        float get_mouse_y_in_window() const;
    private:
        bool block_mouse_{ false };
        bool block_keyboard_{ false };
    };
}
