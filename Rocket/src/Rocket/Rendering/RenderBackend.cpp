module;
module RenderBackend;

import Log;
import RenderCommand;
import WindowsLib;

namespace rke
{
    GraphicsAPI RenderBackend::get_graphics_api()
    {
        // offer alternatives if available
        return GraphicsAPI::OpenGL;
    }

    void RenderBackend::init_window_context(NativeWindow context)
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
        
        void* getter{ get_proc_address_getter() };
        set_proc_address_getter(getter);

        RenderCommand::enable_blend();
        RenderCommand::disable_srgb(); // manually applied in ToneMapping
        RenderCommand::enable_depth_test();
    }
}
