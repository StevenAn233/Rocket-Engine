module;
module Shader;
import :OpenGL;

import Log;
import RenderBackend;
import FileUtils;
import ConfigProxy;

namespace rke
{
    Scope<Shader> Shader::create(const Path& shader_path)
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
        
        switch(render_backend::get_graphics_api())
        {
        case GraphicsAPI::OpenGL:
            return create_scope<glShader>(name, paths);
        default:
            CORE_ASSERT(false, u8"Shader: Other graphics api(s) not supported!");
        }
        return nullptr;
    }
}
