module;
module RendererAPI;

namespace rke
{
    RendererAPI::GraphicAPI RendererAPI::get_graphic_api()
    {
        // offer alternatives if available
        return RendererAPI::GraphicAPI::OpenGL;
    }
}
