module;
module Shader;

import Log;
import FileUtils;
import ConfigProxy;
import HeapManager;
import GShader;

namespace rke
{
    Shader::Shader(const Path& shader_path)
    {
        if(!shader_path.exists()) {
            CORE_ERROR(u8"Shader: Config file '{}' not found!", shader_path);
            return;
        }
        name_ = shader_path.stem().string();
        Path base_dir{ shader_path.parent_path() };

        Scope<ConfigReader> reader{ ConfigReader::create(shader_path) };
        if(reader->has_key(u8"Vertex")) stage_paths_[ShaderStage::Vertex] =
            base_dir / file::unify_path(reader->get_at(u8"Vertex", String{}));
        if(reader->has_key(u8"Fragment")) stage_paths_[ShaderStage::Fragment] =
            base_dir / file::unify_path(reader->get_at(u8"Fragment", String{}));
        if(reader->has_key(u8"Geometry")) stage_paths_[ShaderStage::Geometry] =
            base_dir / file::unify_path(reader->get_at(u8"Geometry", String{}));
        if(reader->has_key(u8"Compute")) stage_paths_[ShaderStage::Compute] =
            base_dir / file::unify_path(reader->get_at(u8"Compute", String{}));
        if(reader->has_key(u8"TessControl")) stage_paths_[ShaderStage::TessControl] =
            base_dir / file::unify_path(reader->get_at(u8"TessControl", String{}));
        if(reader->has_key(u8"TessEvaluation")) stage_paths_[ShaderStage::TessEvaluation] =
            base_dir / file::unify_path(reader->get_at(u8"TessEvaluation", String{}));
    }

    GShader* Shader::get_gshader(const ShaderSettings& settings)
    {
        if(name_.empty()) return nullptr;
        auto it{ gpu_variants_.find(settings) };
        if(it != gpu_variants_.end()) return it->second.get();

        ShaderSources sources{};
        for(const auto& [stage, path] : stage_paths_)
        {
            if(!path.exists()) {
                CORE_ERROR(u8"Shader: Stage path '{}' doesn't exist!", path);
                continue;
            }
            sources[stage] = { path, file::read_file_string(path) };
        }
        if(sources.empty()) return nullptr;

        Scope<GShader> gpu{ GShader::create(name_, sources) };
        CORE_ASSERT(gpu, u8"Shader: Failed to compile '{}'!", name_);

        GShader* raw{ gpu.get() };
        gpu_variants_.emplace(settings, std::move(gpu));
        return raw;
    }
}
