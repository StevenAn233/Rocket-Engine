export module MouseButtons;

import Types;
import String;

export namespace rke
{
    enum class Mouse : uint16
    {
        Button0 = 0,
        Button1 = 1,
        Button2 = 2,
        Button3 = 3,
        Button4 = 4,
        Button5 = 5,
        Button6 = 6,
        Button7 = 7,

        Left    = Button0,
        Right   = Button1,
        Middle  = Button2
    };

    constexpr StringView mouse_button_to_string(Mouse code)
    {
        switch(code)
        {
        case Mouse::Left:    return u8"Mouse Left";
        case Mouse::Right:   return u8"Mouse Right";
        case Mouse::Middle:  return u8"Mouse Middle";
        case Mouse::Button3: return u8"Mouse Button 4";
        case Mouse::Button4: return u8"Mouse Button 5";
        case Mouse::Button5: return u8"Mouse Button 6";
        case Mouse::Button6: return u8"Mouse Button 7";
        case Mouse::Button7: return u8"Mouse Button 8";
        default:             return u8"Unknown Button";
        }
    }
}
