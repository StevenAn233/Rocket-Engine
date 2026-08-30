module;
module GShader;
import :OpenGL;

import Log;
import RenderBackend;

namespace rke
{
    Scope<GShader> GShader::create(const String& name, const ShaderSources& sources)
    {
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glGShader>(name, sources);
        default:
            CORE_ASSERT(false, u8"GShader: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
