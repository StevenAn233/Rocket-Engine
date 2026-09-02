module;
module Shader;

import Log;
import FileUtils;
import ConfigProxy;
import HeapManager;
import GShader;

namespace rke
{
    Shader::Shader(const Path& rkshdr_path)
    {
        if(!rkshdr_path.exists()) {
            CORE_ERROR(u8"Shader: Config file '{}' not found!", rkshdr_path);
            return;
        }
        name_ = rkshdr_path.stem().string();
        Path base_dir{ rkshdr_path.parent_path() };

        Scope<ConfigReader> reader{ ConfigReader::create(rkshdr_path) };
        if(reader->has_key(u8"Vertex"))
            shader_paths_[static_cast<uint32>(ShaderStage::Vertex)] =
                base_dir / file::unify_path(reader->get_at(u8"Vertex", String{}));
        if(reader->has_key(u8"Fragment"))
            shader_paths_[static_cast<uint32>(ShaderStage::Fragment)] =
                base_dir / file::unify_path(reader->get_at(u8"Fragment", String{}));
        if(reader->has_key(u8"Geometry"))
            shader_paths_[static_cast<uint32>(ShaderStage::Geometry)] =
                base_dir / file::unify_path(reader->get_at(u8"Geometry", String{}));
        if(reader->has_key(u8"Compute"))
            shader_paths_[static_cast<uint32>(ShaderStage::Compute)] =
                base_dir / file::unify_path(reader->get_at(u8"Compute", String{}));
        if(reader->has_key(u8"TessControl"))
            shader_paths_[static_cast<uint32>(ShaderStage::TessControl)] =
                base_dir / file::unify_path(reader->get_at(u8"TessControl", String{}));
        if(reader->has_key(u8"TessEvaluation"))
            shader_paths_[static_cast<uint32>(ShaderStage::TessEvaluation)] =
                base_dir / file::unify_path(reader->get_at(u8"TessEvaluation", String{}));
    }

    GShader* Shader::get_gshader(const ShaderSettings& settings)
    {
        if(name_.empty()) return nullptr;
        auto it{ gpu_variants_.find(settings) };
        if(it != gpu_variants_.end()) return it->second.get();

        ShaderPaths valid_paths{ shader_paths_ };
        ShaderSources sources{};
        bool empty{ true };
        for(uint32 stage{}; stage < shader_paths_.size(); stage++)
        {
            Path& path{ valid_paths[stage] };
            if(path.empty()) continue;
            if(!path.exists()) {
                CORE_ERROR(u8"Shader: Stage path '{}' doesn't exist!", path);
                path.clear(); continue;
            }
            sources[stage] = file::read_file_string(path);
            empty = false;
        }
        if(empty) return nullptr;

        Scope<GShader> gpu{ GShader::create(name_, valid_paths, sources) };
        CORE_ASSERT(gpu, u8"Shader: Failed to compile '{}'!", name_);

        GShader* raw{ gpu.get() };
        gpu_variants_.emplace(settings, std::move(gpu));
        return raw;
    }
}
