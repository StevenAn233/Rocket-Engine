export module Keys;

import Types;
import String;

export namespace rke
{
    enum class Key : uint16
    {
        Space          = 32,
        Apostrophe     = 39,
        Comma          = 44,
        Minus          = 45,
        Period         = 46,
        Slash          = 47,
        Num0           = 48,
        Num1           = 49,
        Num2           = 50,
        Num3           = 51,
        Num4           = 52,
        Num5           = 53,
        Num6           = 54,
        Num7           = 55,
        Num8           = 56,
        Num9           = 57,
        Semicolon      = 59,
        Equal          = 61,
        A              = 65,
        B              = 66,
        C              = 67,
        D              = 68,
        E              = 69,
        F              = 70,
        G              = 71,
        H              = 72,
        I              = 73,
        J              = 74,
        K              = 75,
        L              = 76,
        M              = 77,
        N              = 78,
        O              = 79,
        P              = 80,
        Q              = 81,
        R              = 82,
        S              = 83,
        T              = 84,
        U              = 85,
        V              = 86,
        W              = 87,
        X              = 88,
        Y              = 89,
        Z              = 90,
        LeftBracket    = 91,
        Backslash      = 92,
        RightBracket   = 93,
        GraveAccent    = 96,
        World1         = 161,
        World2         = 162,
        Escape         = 256,
        Enter          = 257,
        Tab            = 258,
        Backspace      = 259,
        Insert         = 260,
        Delete         = 261,
        Right          = 262,
        Left           = 263,
        Down           = 264,
        Up             = 265,
        PageUp         = 266,
        PageDown       = 267,
        Home           = 268,
        End            = 269,
        CapsLock       = 280,
        ScrollLock     = 281,
        NumLock        = 282,
        PrintScreen    = 283,
        Pause          = 284,
        F1             = 290,
        F2             = 291,
        F3             = 292,
        F4             = 293,
        F5             = 294,
        F6             = 295,
        F7             = 296,
        F8             = 297,
        F9             = 298,
        F10            = 299,
        F11            = 300,
        F12            = 301,
        F13            = 302,
        F14            = 303,
        F15            = 304,
        F16            = 305,
        F17            = 306,
        F18            = 307,
        F19            = 308,
        F20            = 309,
        F21            = 310,
        F22            = 311,
        F23            = 312,
        F24            = 313,
        F25            = 314,
        Keypad0        = 320,
        Keypad1        = 321,
        Keypad2        = 322,
        Keypad3        = 323,
        Keypad4        = 324,
        Keypad5        = 325,
        Keypad6        = 326,
        Keypad7        = 327,
        Keypad8        = 328,
        Keypad9        = 329,
        KeypadDecimal  = 330,
        KeypadDivide   = 331,
        KeypadMultiply = 332,
        KeypadSubstact = 333,
        KeypadAdd      = 334,
        KeypadEnter    = 335,
        KeypadEqual    = 336,
        LeftShift      = 340,
        LeftControl    = 341,
        LeftAlt        = 342,
        LeftSuper      = 343,
        RightShift     = 344,
        RightControl   = 345,
        RightAlt       = 346,
        RightSuper     = 347,
        Menu           = 348
    };

    constexpr StringView key_to_string(Key code)
    {
        switch(code)
        {
        case Key::Space:          return u8"Space";
        case Key::Apostrophe:     return u8"Apostrophe";
        case Key::Comma:          return u8"Comma";
        case Key::Minus:          return u8"Minus";
        case Key::Period:         return u8"Period";
        case Key::Slash:          return u8"Slash";
        case Key::Num0:           return u8"0";               
        case Key::Num1:           return u8"1";               
        case Key::Num2:           return u8"2";               
        case Key::Num3:           return u8"3";               
        case Key::Num4:           return u8"4";               
        case Key::Num5:           return u8"5";               
        case Key::Num6:           return u8"6";               
        case Key::Num7:           return u8"7";               
        case Key::Num8:           return u8"8";               
        case Key::Num9:           return u8"9";               
        case Key::Semicolon:      return u8"Semicolon";
        case Key::Equal:          return u8"Equal";
        case Key::A:              return u8"A";               
        case Key::B:              return u8"B";               
        case Key::C:              return u8"C";               
        case Key::D:              return u8"D";               
        case Key::E:              return u8"E";               
        case Key::F:              return u8"F";               
        case Key::G:              return u8"G";               
        case Key::H:              return u8"H";               
        case Key::I:              return u8"I";               
        case Key::J:              return u8"J";               
        case Key::K:              return u8"K";               
        case Key::L:              return u8"L";               
        case Key::M:              return u8"M";               
        case Key::N:              return u8"N";               
        case Key::O:              return u8"O";               
        case Key::P:              return u8"P";               
        case Key::Q:              return u8"Q";               
        case Key::R:              return u8"R";               
        case Key::S:              return u8"S";               
        case Key::T:              return u8"T";               
        case Key::U:              return u8"U";               
        case Key::V:              return u8"V";               
        case Key::W:              return u8"W";               
        case Key::X:              return u8"X";               
        case Key::Y:              return u8"Y";               
        case Key::Z:              return u8"Z";               
        case Key::LeftBracket:    return u8"Left Bracket";
        case Key::Backslash:      return u8"Backslash";
        case Key::RightBracket:   return u8"Right Bracket";
        case Key::GraveAccent:    return u8"Grave Accent";
        case Key::World1:         return u8"World1";
        case Key::World2:         return u8"World2";
        case Key::Escape:         return u8"Escape";
        case Key::Enter:          return u8"Enter";
        case Key::Tab:            return u8"Tab";
        case Key::Backspace:      return u8"Backspace";
        case Key::Insert:         return u8"Insert";
        case Key::Delete:         return u8"Delete";
        case Key::Right:          return u8"Right";
        case Key::Left:           return u8"Left";
        case Key::Down:           return u8"Down";
        case Key::Up:             return u8"Up";
        case Key::PageUp:         return u8"Page Up";
        case Key::PageDown:       return u8"Page Down";
        case Key::Home:           return u8"Home";
        case Key::End:            return u8"End";
        case Key::CapsLock:       return u8"Caps Lock";
        case Key::ScrollLock:     return u8"Scroll Lock";
        case Key::NumLock:        return u8"Num Lock";
        case Key::PrintScreen:    return u8"Print Screen";
        case Key::Pause:          return u8"Pause";
        case Key::F1:             return u8"F1";              
        case Key::F2:             return u8"F2";              
        case Key::F3:             return u8"F3";              
        case Key::F4:             return u8"F4";              
        case Key::F5:             return u8"F5";              
        case Key::F6:             return u8"F6";              
        case Key::F7:             return u8"F7";              
        case Key::F8:             return u8"F8";              
        case Key::F9:             return u8"F9";              
        case Key::F10:            return u8"F10";             
        case Key::F11:            return u8"F11";             
        case Key::F12:            return u8"F12";             
        case Key::F13:            return u8"F13";             
        case Key::F14:            return u8"F14";             
        case Key::F15:            return u8"F15";             
        case Key::F16:            return u8"F16";             
        case Key::F17:            return u8"F17";             
        case Key::F18:            return u8"F18";             
        case Key::F19:            return u8"F19";             
        case Key::F20:            return u8"F20";             
        case Key::F21:            return u8"F21";             
        case Key::F22:            return u8"F22";             
        case Key::F23:            return u8"F23";             
        case Key::F24:            return u8"F24";             
        case Key::F25:            return u8"F25";             
        case Key::Keypad0:        return u8"Keypad 0";        
        case Key::Keypad1:        return u8"Keypad 1";        
        case Key::Keypad2:        return u8"Keypad 2";        
        case Key::Keypad3:        return u8"Keypad 3";        
        case Key::Keypad4:        return u8"Keypad 4";        
        case Key::Keypad5:        return u8"Keypad 5";        
        case Key::Keypad6:        return u8"Keypad 6";        
        case Key::Keypad7:        return u8"Keypad 7";        
        case Key::Keypad8:        return u8"Keypad 8";        
        case Key::Keypad9:        return u8"Keypad 9";        
        case Key::KeypadDecimal:  return u8"Keypad Decimal";
        case Key::KeypadDivide:   return u8"Keypad Divide";
        case Key::KeypadMultiply: return u8"Keypad Multiply";
        case Key::KeypadSubstact: return u8"Keypad Substact";
        case Key::KeypadAdd:      return u8"Keypad Add";
        case Key::KeypadEnter:    return u8"Keypad Enter";
        case Key::KeypadEqual:    return u8"Keypad Equal";
        case Key::LeftShift:      return u8"Left Shift";
        case Key::LeftControl:    return u8"Left Control";
        case Key::LeftAlt:        return u8"Left Alt";
        case Key::LeftSuper:      return u8"Left Super";
        case Key::RightShift:     return u8"Right Shift";
        case Key::RightControl:   return u8"Right Control";
        case Key::RightAlt:       return u8"Right Alt";
        case Key::RightSuper:     return u8"Right Super";
        case Key::Menu:           return u8"Menu";
        default:                  return u8"Unknown Key";
        }
    }
}
