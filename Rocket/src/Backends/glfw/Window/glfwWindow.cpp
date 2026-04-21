module;

#include <GLFW/glfw3.h>
#include <stb_image.h>

module Window;
import :glfw;

import Log;
import Keys;
import MouseButtons;
import RenderCommand;
import Instrumentor;
import FileUtils;

namespace {
    using namespace rke;

    constexpr Mouse glfw_to_rke_mouse(int glfw_code)
    {
        switch(glfw_code)
        {
        case GLFW_MOUSE_BUTTON_LEFT:   return Mouse::Left;
        case GLFW_MOUSE_BUTTON_RIGHT:  return Mouse::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return Mouse::Middle;
        case GLFW_MOUSE_BUTTON_4:      return Mouse::Button3;
        case GLFW_MOUSE_BUTTON_5:      return Mouse::Button4;
        case GLFW_MOUSE_BUTTON_6:      return Mouse::Button5;
        case GLFW_MOUSE_BUTTON_7:      return Mouse::Button6;
        case GLFW_MOUSE_BUTTON_8:      return Mouse::Button7;
        }
        return Mouse::Button0;
    }

    constexpr Key glfw_to_rke_key(int glfw_code)
    {
        switch(glfw_code)
        {
        case GLFW_KEY_SPACE:         return Key::Space;
        case GLFW_KEY_APOSTROPHE:    return Key::Apostrophe;
        case GLFW_KEY_COMMA:         return Key::Comma;
        case GLFW_KEY_MINUS:         return Key::Minus;
        case GLFW_KEY_PERIOD:        return Key::Period;
        case GLFW_KEY_SLASH:         return Key::Slash;
        case GLFW_KEY_0:             return Key::Num0;
        case GLFW_KEY_1:             return Key::Num1;
        case GLFW_KEY_2:             return Key::Num2;           
        case GLFW_KEY_3:             return Key::Num3;           
        case GLFW_KEY_4:             return Key::Num4;           
        case GLFW_KEY_5:             return Key::Num5;           
        case GLFW_KEY_6:             return Key::Num6;           
        case GLFW_KEY_7:             return Key::Num7;           
        case GLFW_KEY_8:             return Key::Num8;           
        case GLFW_KEY_9:             return Key::Num9;           
        case GLFW_KEY_SEMICOLON:     return Key::Semicolon;
        case GLFW_KEY_EQUAL:         return Key::Equal;
        case GLFW_KEY_A:             return Key::A;
        case GLFW_KEY_B:             return Key::B;              
        case GLFW_KEY_C:             return Key::C;              
        case GLFW_KEY_D:             return Key::D;              
        case GLFW_KEY_E:             return Key::E;              
        case GLFW_KEY_F:             return Key::F;              
        case GLFW_KEY_G:             return Key::G;              
        case GLFW_KEY_H:             return Key::H;              
        case GLFW_KEY_I:             return Key::I;              
        case GLFW_KEY_J:             return Key::J;              
        case GLFW_KEY_K:             return Key::K;              
        case GLFW_KEY_L:             return Key::L;              
        case GLFW_KEY_M:             return Key::M;              
        case GLFW_KEY_N:             return Key::N;              
        case GLFW_KEY_O:             return Key::O;              
        case GLFW_KEY_P:             return Key::P;              
        case GLFW_KEY_Q:             return Key::Q;              
        case GLFW_KEY_R:             return Key::R;              
        case GLFW_KEY_S:             return Key::S;              
        case GLFW_KEY_T:             return Key::T;              
        case GLFW_KEY_U:             return Key::U;              
        case GLFW_KEY_V:             return Key::V;              
        case GLFW_KEY_W:             return Key::W;              
        case GLFW_KEY_X:             return Key::X;              
        case GLFW_KEY_Y:             return Key::Y;              
        case GLFW_KEY_Z:             return Key::Z;              
        case GLFW_KEY_LEFT_BRACKET:  return Key::LeftBracket;
        case GLFW_KEY_BACKSLASH:     return Key::Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return Key::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:  return Key::GraveAccent;
        case GLFW_KEY_WORLD_1:       return Key::World1;         
        case GLFW_KEY_WORLD_2:       return Key::World2;         
        case GLFW_KEY_ESCAPE:        return Key::Escape;         
        case GLFW_KEY_ENTER:         return Key::Enter;
        case GLFW_KEY_TAB:           return Key::Tab;
        case GLFW_KEY_BACKSPACE:     return Key::Backspace;     
        case GLFW_KEY_INSERT:        return Key::Insert;
        case GLFW_KEY_DELETE:        return Key::Delete;
        case GLFW_KEY_RIGHT:         return Key::Right;
        case GLFW_KEY_LEFT:          return Key::Left;
        case GLFW_KEY_DOWN:          return Key::Down;
        case GLFW_KEY_UP:            return Key::Up;
        case GLFW_KEY_PAGE_UP:       return Key::PageUp;
        case GLFW_KEY_PAGE_DOWN:     return Key::PageDown;
        case GLFW_KEY_HOME:          return Key::Home;
        case GLFW_KEY_END:           return Key::End;
        case GLFW_KEY_CAPS_LOCK:     return Key::CapsLock;
        case GLFW_KEY_SCROLL_LOCK:   return Key::ScrollLock;
        case GLFW_KEY_NUM_LOCK:      return Key::NumLock;
        case GLFW_KEY_PRINT_SCREEN:  return Key::PrintScreen;
        case GLFW_KEY_PAUSE:         return Key::Pause;
        case GLFW_KEY_F1:            return Key::F1;
        case GLFW_KEY_F2:            return Key::F2;
        case GLFW_KEY_F3:            return Key::F3;
        case GLFW_KEY_F4:            return Key::F4;
        case GLFW_KEY_F5:            return Key::F5;             
        case GLFW_KEY_F6:            return Key::F6;             
        case GLFW_KEY_F7:            return Key::F7;             
        case GLFW_KEY_F8:            return Key::F8;             
        case GLFW_KEY_F9:            return Key::F9;             
        case GLFW_KEY_F10:           return Key::F10;            
        case GLFW_KEY_F11:           return Key::F11;            
        case GLFW_KEY_F12:           return Key::F12;            
        case GLFW_KEY_F13:           return Key::F13;            
        case GLFW_KEY_F14:           return Key::F14;            
        case GLFW_KEY_F15:           return Key::F15;            
        case GLFW_KEY_F16:           return Key::F16;            
        case GLFW_KEY_F17:           return Key::F17;            
        case GLFW_KEY_F18:           return Key::F18;            
        case GLFW_KEY_F19:           return Key::F19;            
        case GLFW_KEY_F20:           return Key::F20;            
        case GLFW_KEY_F21:           return Key::F21;            
        case GLFW_KEY_F22:           return Key::F22;            
        case GLFW_KEY_F23:           return Key::F23;            
        case GLFW_KEY_F24:           return Key::F24;            
        case GLFW_KEY_F25:           return Key::F25;            
        case GLFW_KEY_KP_0:          return Key::Keypad0;        
        case GLFW_KEY_KP_1:          return Key::Keypad1;        
        case GLFW_KEY_KP_2:          return Key::Keypad2;        
        case GLFW_KEY_KP_3:          return Key::Keypad3;        
        case GLFW_KEY_KP_4:          return Key::Keypad4;        
        case GLFW_KEY_KP_5:          return Key::Keypad5;        
        case GLFW_KEY_KP_6:          return Key::Keypad6;        
        case GLFW_KEY_KP_7:          return Key::Keypad7;        
        case GLFW_KEY_KP_8:          return Key::Keypad8;        
        case GLFW_KEY_KP_9:          return Key::Keypad9;        
        case GLFW_KEY_KP_DECIMAL:    return Key::KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE:     return Key::KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY:   return Key::KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT:   return Key::KeypadSubstact;
        case GLFW_KEY_KP_ADD:        return Key::KeypadAdd;
        case GLFW_KEY_KP_ENTER:      return Key::KeypadEnter;
        case GLFW_KEY_KP_EQUAL:      return Key::KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT:    return Key::LeftShift;
        case GLFW_KEY_LEFT_CONTROL:  return Key::LeftControl;
        case GLFW_KEY_LEFT_ALT:      return Key::LeftAlt;
        case GLFW_KEY_LEFT_SUPER:    return Key::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:   return Key::RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return Key::RightControl;
        case GLFW_KEY_RIGHT_ALT:     return Key::RightAlt;
        case GLFW_KEY_RIGHT_SUPER:   return Key::RightSuper;
        case GLFW_KEY_MENU:          return Key::Menu;
        }
        return Key::Space;
    }
}

namespace rke
{
    void glfwWindow::swap_buffers()
    {
        RKE_PROFILE_FUNCTION();
        context_->swap_buffers();
    }

    void glfwWindow::on_event(Event& e)
    {
        if(data_.title != e.get_window_title()) return;

        for(auto it{ layer_stack_.rbegin() }; it != layer_stack_.rend(); ++it)
        {
            if(e.handled()) return;
            (*it)->on_event(e);
            // if upper layer(overlays first) has handled event
            // then break(do not let other layers deal with it)
        }
    }

    void glfwWindow::on_update(float dt)
    {
        check_layer_blocking();
        make_context_current();

        RenderCommand::set_viewport(0, 0, get_width(), get_height());

        for(const auto& layer : layer_stack_)
            layer->on_update(dt);
    }

    void glfwWindow::on_render()
    {
        if(minimized()) return;
        for(const auto& layer : layer_stack_)
            layer->on_render();
    }

    void glfwWindow::on_imgui_render()
    {
        RKE_PROFILE_FUNCTION();
        for(auto it{ layer_stack_.rbegin() }; it < layer_stack_.rend(); ++it)
            (*it)->on_imgui_render();
    }

    std::pair<int, int> glfwWindow::get_window_pos() const
    {
        int x{}, y{};
        glfwGetWindowPos(window_, &x, &y);
        return { x, y };
    }

    void glfwWindow::push_layer  (Scope<Layer> layer  )
        { layer_stack_.push_layer  (std::move(layer  )); }
    void glfwWindow::push_overlay(Scope<Layer> overlay)
        { layer_stack_.push_overlay(std::move(overlay)); }
    Scope<Layer> glfwWindow::pop_layer  (Layer* layer  )
        { return layer_stack_.pop_layer  (layer  ); }
    Scope<Layer> glfwWindow::pop_overlay(Layer* overlay)
        { return layer_stack_.pop_overlay(overlay); }

    void glfwWindow::update_vsync()
    {
        make_context_current();
        if(data_.vsync_extent > 1.0f)
        {
            CORE_WARN(u8"glfwWindow: V-sync extent can only be between 0.0 to 1.0.");
            CORE_INFO(u8"glfwWindow: V-sync already set to 1.0(on).");
            glfwSwapInterval(1);
            data_.vsync_extent = 1.0f;
        }
        else if(std::abs(data_.vsync_extent) < 0.0001f)
        {
            glfwSwapInterval(0);
            data_.vsync_extent = 0.0f;
        }
        else glfwSwapInterval(static_cast<int>(1.0f / data_.vsync_extent) + 0.5f);
    }

    void glfwWindow::make_context_current() { glfwMakeContextCurrent(window_); }

    void glfwWindow::check_layer_blocking()
    {
        RKE_PROFILE_FUNCTION();

        mouse_blocking_layer_index_	   = 0;
        keyboard_blocking_layer_index_ = 0;

        for(auto it{ layer_stack_.rbegin() }; it != layer_stack_.rend(); ++it)
        {
            if((*it)->should_block_mouse())
            {
                mouse_blocking_layer_index_ = (*it)->get_index();
                return;
            }
            if((*it)->should_block_keyboard())
            {
                keyboard_blocking_layer_index_ = (*it)->get_index();
                return;
            }
        }
    }

    void glfwWindow::init(const Window::WindowProps& props, NativeWindow shared_handle)
    {
        data_.title  = props.title;
        data_.width  = props.width;
        data_.height = props.height;

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_SAMPLES, 4);			   // IMPORTANT
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_FALSE); // Manually applied in ToneMapping
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE); // imgui bug
        window_ = glfwCreateWindow
        (
            static_cast<int>(props.width ),
            static_cast<int>(props.height),
            props.title.raw(), nullptr,
            shared_handle.as<GLFWwindow>()
        );
        CORE_ASSERT(window_, u8"glfwWindow: Failed to create window '{}'!", props.title);
        glfwSetWindowPos(window_, props.x_coord, props.y_coord);
        glfwShowWindow(window_);

    // set OpenGL context
        context_ = Context::create(NativeWindow(window_));
        context_->init();

    // set window icon(after glfwCreateWindow)
        if(props.icon_path.exists())
        {
            int width{}, height{}, channels{};
            stbi_uc* pixels{ stbi_load
            (
                props.icon_path.string().raw(),
                &width, &height, &channels, 4
            )};
            CORE_ASSERT(pixels, u8"glfwWindow: Failed to load window icon!");

            GLFWimage images[1]{};
            images[0].width  = width;
            images[0].height = height;
            images[0].pixels = pixels;

            glfwSetWindowIcon(window_, 1, images);
        }
        else if(!props.icon_path.empty()) CORE_ERROR
            (u8"glfwWidnow: Icon path '{}' doesn't exist!", props.icon_path.string());

    // set vsync
        data_.vsync_extent = 1.0f;
        update_vsync();

        glfwSetWindowUserPointer(window_, &data_);
    /*------------------------------Set GLFW Callbacks------------------------------*/
    /*-----------GLFW Callbacks will be called automatically as you input-----------*/

        // glfw callbacks are used to pass event-datas into [event_callback] function
        // and datas will be processed by [event_callback] function to for example,
        // make movements, change field of view, rotate perspective, etc.

        glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height)
        {
            auto* data_ptr{ (WindowData*)glfwGetWindowUserPointer(window) };
            // [data] here is a pointer to my object member [data_]
            // basically here we are RESETting our [data_.width] && [data_.height]
            // with the parameters[int width, int height]
            data_ptr->width  = width;
            data_ptr->height = height;
            if(data_ptr->width == 0 || data_ptr->height == 0)
                data_ptr->minimized = true;
            else data_ptr->minimized = false;
            WindowResizedEvent event
            {
                data_ptr->title,
                static_cast<uint32>(width ),
                static_cast<uint32>(height)
            };
            data_ptr->event_callback(event);
        });

        glfwSetWindowCloseCallback(window_, [](GLFWwindow* window)
        {
            auto* data_ptr{ (WindowData*)glfwGetWindowUserPointer(window) };
            WindowClosedEvent event(data_ptr->title);
            data_ptr->event_callback(event);
        });

        glfwSetKeyCallback(window_,
        [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            auto* data_ptr{ (WindowData*)glfwGetWindowUserPointer(window) };

            switch(action)
            {
            case GLFW_PRESS:
            {
                KeyPressedEvent event{ data_ptr->title, glfw_to_rke_key(key) };
                data_ptr->event_callback(event);
            } break;
            case GLFW_REPEAT:
            {
                KeyPressedEvent event{ data_ptr->title, glfw_to_rke_key(key), true };
                data_ptr->event_callback(event);
            } break;
            case GLFW_RELEASE:
            {
                KeyReleasedEvent event{ data_ptr->title, glfw_to_rke_key(key) };
                data_ptr->event_callback(event);
            } break;
            }
        });

        glfwSetCharCallback(window_, [](GLFWwindow* window, uint32 codepoint)
        {
            auto* data_ptr{ (WindowData*)glfwGetWindowUserPointer(window) };
            CharTypedEvent event{ data_ptr->title, codepoint };
            data_ptr->event_callback(event);
        });

        glfwSetMouseButtonCallback(window_,
        [](GLFWwindow* window, int button, int action, int mods)
        {
            auto* data_ptr{ (WindowData*)glfwGetWindowUserPointer(window) };
            switch(action)
            {
            case GLFW_PRESS:
            {
                MouseButtonPressedEvent event{ data_ptr->title, glfw_to_rke_mouse(button) };
                data_ptr->event_callback(event);
            } break;
            case GLFW_RELEASE:
            {
                MouseButtonReleasedEvent event{ data_ptr->title, glfw_to_rke_mouse(button) };
                data_ptr->event_callback(event);
            } break;
            }
        });

        glfwSetScrollCallback(window_,
        [](GLFWwindow* window, double x_offset, double y_offset)
        {
            auto* data_ptr{ (WindowData*)glfwGetWindowUserPointer(window) };

            MouseScrolledEvent event{ data_ptr->title, (float)x_offset, (float)y_offset };
            data_ptr->event_callback(event);
        });

        glfwSetCursorPosCallback(window_,
        [](GLFWwindow* window, double x_coord, double y_coord)
        {
            auto* data_ptr{ (WindowData*)glfwGetWindowUserPointer(window) };

            MouseMovedEvent event{ data_ptr->title, (float)x_coord, (float)y_coord };
            data_ptr->event_callback(event);
        });
    }

    void glfwWindow::shutdown()
    {
        glfwSetWindowShouldClose(window_, 1);
        glfwDestroyWindow(window_);
    }
}
