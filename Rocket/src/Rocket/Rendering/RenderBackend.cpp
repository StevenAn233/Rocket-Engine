module;
module RenderBackend;

import Log;
import RenderCommand;
import WindowsLib;
import Application;

namespace rke::render_backend
{
    GraphicsAPI get_graphics_api()
    {
        // offer alternatives if available
        return GraphicsAPI::OpenGL;
    }

    void init_window_context(NativeWindow context)
    {
        switch(get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            if(!WindowsLib::is_context_current(context))
            {
                CORE_ERROR(u8"glfwWindow: This context is not currrent!");
                return;
            }
        }
        
        void* getter{ internal::get_proc_address_getter() };
        internal::set_proc_address_getter(getter);

        app().render_command().enable_blend();
        app().render_command().disable_srgb(); // manually applied in ToneMapping
        app().render_command().enable_depth_test();
    }
}
