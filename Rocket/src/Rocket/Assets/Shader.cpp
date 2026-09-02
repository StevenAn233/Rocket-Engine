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
        ShaderHasher hasher{};
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
            String content{ file::read_file_string(path) };
            uint64 fingerprint{ hasher.hash(path, content) };
            sources[stage].content = std::move(content);
            sources[stage].fingerprint = fingerprint;
            empty = false;
        }
        if(empty) return nullptr;

        Scope<GShader> gpu{ GShader::create(name_, valid_paths, sources) };
        CORE_ASSERT(gpu, u8"Shader: Failed to compile '{}'!", name_);

        GShader* raw{ gpu.get() };
        gpu_variants_.emplace(settings, std::move(gpu));
        return raw;
    }

// ShaderHasher
    uint64 ShaderHasher::hash(const Path& path, const String& source)
    {
        uint64 hash{ fnv1a_offset_basis };
        hash_source_recursive(hash, path, source, 0);
        return hash;
    }

    void ShaderHasher::hash_source_recursive(uint64& hash,
        const Path& source_path, const String& source, uint32 depth)
    {
        hash = fnv1a_string(hash, source);
        if(depth >= 6u) return;

        Size line_begin{};
        const Size len{ source.length() };
        while(line_begin < len)
        {
            Size line_end{ source.find(u8"\n", line_begin) };
            if(line_end == String::npos) line_end = len;

            Size inc_begin{}, inc_end{};
            if(try_parse_include_line(source, line_begin, line_end, inc_begin, inc_end))
            {
                Path inc_path{ source_path.parent_path() /
                    Path(source.substr(inc_begin, inc_end - inc_begin)) };
                auto it{ cached_sources_.find(inc_path) };
                if(it != cached_sources_.end())
                    hash_source_recursive(hash, inc_path, it->second, depth + 1);
                else {
                    if(inc_path.exists()) {
                        String inc_content{ file::read_file_string(inc_path) };
                        it = cached_sources_.emplace(inc_path, std::move(inc_content)).first;
                        hash_source_recursive(hash, inc_path, it->second, depth + 1);
                    }
                    else CORE_WARN(u8"glGShader: Include '{}' not found while "
                        u8"fingerprinting '{}'!", inc_path, source_path);
                }
            }
            if(line_end == len) break;
            line_begin = line_end + 1;
        }
    }

    uint64 ShaderHasher::fnv1a_update(uint64 hash, const void* data, Size size)
    {
        const unsigned char* bytes{ static_cast<const unsigned char*>(data) };
        for(Size i{}; i < size; i++)
        {
            hash ^= bytes[i];
            hash *= fnv1a_prime;
        }
        return hash;
    }

    bool ShaderHasher::try_parse_include_line(const String& source,
        Size line_begin, Size line_end, Size& out_begin, Size& out_end)
    {
        Size i{ line_begin };
        const Size len{ line_end };
        while(i < len && (source[i] == u8' ' || source[i] == u8'\t')) i++;
        if(len - i < 8) return false;
        constexpr const char8 include_kw[8]
            { u8'#', u8'i', u8'n', u8'c', u8'l', u8'u', u8'd', u8'e' };
        for(Size k{}; k < 8; k++)
            if(source[i + k] != include_kw[k]) return false;
        i += 8;
        while(i < len && (source[i] == u8' ' || source[i] == u8'\t')) i++;
        if(i >= len || source[i] != u8'"') return false;
        i++;
        out_begin = i;
        while(i < len && source[i] != u8'"') i++;
        if(i >= len) return false;
        out_end = i;
        return true;
    }
}
