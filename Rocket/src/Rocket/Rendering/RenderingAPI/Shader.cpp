module;
module Shader;
import :OpenGL;

import Log;
import RendererAPI;
import FileUtils;
import ConfigProxy;

namespace rke
{
    Ref<Shader> Shader::create(const Path& shader_path)
    {
        if(!shader_path.exists()) {
            CORE_ERROR(u8"Shader: Config file '{}' not found!", shader_path);
            return nullptr;
        }
        ShaderPathMap paths{};
        String name{ shader_path.stem().string() };
        Path base_dir{ shader_path.parent_path() };

        Scope<ConfigReader> reader{ ConfigReader::create(shader_path) };
        if(reader->has_key(u8"Vertex"  )) paths[ShaderStage::Vertex  ] =
            base_dir / file::unify_path(reader->get_at(u8"Vertex"  , String{}));
        if(reader->has_key(u8"Fragment")) paths[ShaderStage::Fragment] =
            base_dir / file::unify_path(reader->get_at(u8"Fragment", String{}));
        if(reader->has_key(u8"Geometry")) paths[ShaderStage::Geometry] =
            base_dir / file::unify_path(reader->get_at(u8"Geometry", String{}));
        if(reader->has_key(u8"Compute" )) paths[ShaderStage::Compute ] =
            base_dir / file::unify_path(reader->get_at(u8"Compute" , String{}));
        if(reader->has_key(u8"TessControl")) paths[ShaderStage::TessControl] =
            base_dir / file::unify_path(reader->get_at(u8"TessControl", String{}));
        if(reader->has_key(u8"TessEvaluation")) paths[ShaderStage::TessEvaluation] =
            base_dir / file::unify_path(reader->get_at(u8"TessEvaluation", String{}));
        
        switch(RendererAPI::get_graphic_api())
        {
        case RendererAPI::GraphicAPI::OpenGL:
            return create_ref<glShader>(name, paths);
    //	case RendererAPI::GraphicAPI::Vulkan: ...
    //	case RendererAPI::GraphicAPI::DirectX: ...
        case RendererAPI::GraphicAPI::None:
            CORE_ASSERT(false, u8"Renderer: no graphic api support!");
            std::unreachable();
        default:
            CORE_ASSERT(false, u8"Renderer: unknown graphic api!");
            std::unreachable();
        }
        return nullptr;
    }
}
