module;
module RenderBackend;

namespace rke
{
    GraphicsAPI RenderBackend::get_graphics_api()
    {
        // offer alternatives if available
        return GraphicsAPI::OpenGL;
    }
}
