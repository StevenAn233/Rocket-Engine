module;

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <glad/glad.h>

module GShader;
import :OpenGL;

import Log;
import FileUtils;

namespace {
    using namespace rke;

    static inline GLenum to_gl_enum(ShaderStage stage)
    {
        switch(stage)
        {
        case ShaderStage::Vertex:         return GL_VERTEX_SHADER;
        case ShaderStage::Fragment:       return GL_FRAGMENT_SHADER;
        case ShaderStage::Geometry:       return GL_GEOMETRY_SHADER;
        case ShaderStage::Compute:        return GL_COMPUTE_SHADER;
        case ShaderStage::TessControl:    return GL_TESS_CONTROL_SHADER;
        case ShaderStage::TessEvaluation: return GL_TESS_EVALUATION_SHADER;
        }
        CORE_ASSERT(false, u8"glGShader: Unknown shader stage!");
        return 0;
    }

    static inline shaderc_shader_kind to_shaderc(ShaderStage stage)
    {
        switch(stage)
        {
        case ShaderStage::Vertex:         return shaderc_glsl_vertex_shader;
        case ShaderStage::Fragment:       return shaderc_glsl_fragment_shader;
        case ShaderStage::Geometry:       return shaderc_glsl_geometry_shader;
        case ShaderStage::Compute:        return shaderc_glsl_compute_shader;
        case ShaderStage::TessControl:    return shaderc_glsl_tess_control_shader;
        case ShaderStage::TessEvaluation: return shaderc_glsl_tess_evaluation_shader;
        }
        CORE_ASSERT(false, u8"glGShader: Unknown shader stage!");
        return static_cast<shaderc_shader_kind>(0);
    }

    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:
        shaderc_include_result* GetInclude (
            const char* requested_source , shaderc_include_type type,
            const char* requesting_source, Size include_depth) override
        {
            Path requested_path { str::to_char8(requested_source ) };
            Path requesting_path{ str::to_char8(requesting_source) };

            Path final_path{ requesting_path.parent_path() / requested_path };
            CORE_ASSERT(final_path.exists(),
                u8"ShaderIncluder: Path '{}' doesn't exist!", final_path);

            IncludeData* data{ new IncludeData {
                .source_name = String(str::to_char8(requested_source)),
                .content = file::read_file_string(final_path)
            }};
            auto* result { new shaderc_include_result() };
            result->source_name = data->source_name.raw();
            result->source_name_length = data->source_name.length();
            result->content = data->content.raw(); // make sure won't be deleted when getting out of the scope
            result->content_length = data->content.length();
            result->user_data = nullptr;

            included_files_[result] = data; // save to delete

            return result;
        }

        void ReleaseInclude(shaderc_include_result* data) override
        {
            auto it{ included_files_.find(data) };
            if(it != included_files_.end())
            {
                delete it->second;
                delete it->first;
                included_files_.erase(it);
            }
        }
    private:
        struct IncludeData
        {
            String source_name;
            String content;
        };
        std::unordered_map<shaderc_include_result*, IncludeData*> included_files_{};
    };

    static String to_hex16(uint64 value)
    {
        char8 buffer[17]{};
        constexpr char8 digits[16]{ u8'0', u8'1', u8'2', u8'3', u8'4', u8'5',
            u8'6', u8'7', u8'8', u8'9', u8'a', u8'b', u8'c', u8'd', u8'e', u8'f' };
        for(int i{ 15 }; i >= 0; i--)
        {
            buffer[i] = digits[value & 0xF];
            value >>= 4;
        }
        buffer[16] = u8'\0';
        return String(buffer, 16);
    }

    // cache file name embeds the shader name, stage file name and a content
    // fingerprint, so edits to a source or its includes produce a new key
    static String stage_cache_file_name(const String& shader_name,
        const Path& stage_path, uint64 fingerprint, const String& suffix)
    {
        return shader_name + u8"-" +
               stage_path.filename().string() + u8"." +
               to_hex16(fingerprint) + u8"." + suffix;
    }

    static void prune_orphan_caches(const Path& cache_directory, const Path& name_to_keep)
    {
        if(!cache_directory.exists()) return;
        const String suffix{ name_to_keep.extension().string() };
        const String prefix{ name_to_keep.stem().stem().string() };
        try {
            for(const auto& entry : fs::directory_iterator(cache_directory.get()))
            {
                if(!entry.is_regular_file()) continue;
                const Path filename{ entry.path().filename() };
                if(filename == name_to_keep) continue;

            // not auto-gen spirv file: don't delete
                const String name{ filename.string() };
                const bool stale_new_format
                {
                    name.find(prefix) == 0 &&
                    name.ends_with(suffix.c_str())
                };
                if(!stale_new_format) continue;

                fs::remove(entry.path());
                CORE_INFO(u8"glGShader: Pruned stale cache '{}'.", name);
            }
        } catch(const std::exception& e) {
            CORE_WARN(u8"glGShader: Failed to prune cache directory '{}': '{}'.",
                cache_directory, e.what());
        }
    }
}

namespace rke
{
    glGShader::glGShader(String name, const ShaderPaths& paths, const ShaderSources& sources)
        : name_(std::move(name))
    {
        compile_or_get_vulkan_spirv(paths, sources);
        compile_or_get_opengl_spirv(paths, sources);
        create_program();
    }

    glGShader::~glGShader() { glDeleteProgram(gal_id_); }

    void glGShader::bind() const { glUseProgram(gal_id_); }
    void glGShader::unbind() const { glUseProgram(0); }

    const GShader& glGShader::upload(StringView name, int v) const
    {
        glUniform1i(get_uniform_location(name), v);
        return *this;
    }
    const GShader& glGShader::upload(StringView name, float v) const
    {
        glUniform1f(get_uniform_location(name), v);
        return *this;
    }
    const GShader& glGShader::upload(StringView name, int count, int* data) const
    {
        glUniform1iv(get_uniform_location(name), count, data);
        return *this;
    }
    const GShader& glGShader::upload(StringView name, float v0, float v1, float v2) const
    {
        glUniform3f(get_uniform_location(name), v0, v1, v2);
        return *this;
    }
    const GShader& glGShader::upload(StringView name, glm::vec2 vec) const
    {
        glUniform2f(get_uniform_location(name), vec.x, vec.y);
        return *this;
    }
    const GShader& glGShader::upload(StringView name, glm::vec3 vec) const
    {
        glUniform3f(get_uniform_location(name), vec.x, vec.y, vec.z);
        return *this;
    }
    const GShader& glGShader::upload(StringView name, glm::vec4 vec) const
    {
        glUniform4f(get_uniform_location(name), vec[0], vec[1], vec[2], vec[3]);
        return *this;
    }
    const GShader& glGShader::upload(StringView name,
        float v0, float v1, float v2, float v3) const
    {
        glUniform4f(get_uniform_location(name), v0, v1, v2, v3);
        return *this;
    }

    int glGShader::get_uniform_location(const String& name) const
    {
        auto it{ uniform_location_cache_.find(name) };
        if(it != uniform_location_cache_.end()) return it->second;

        GLint location{ glGetUniformLocation(gal_id_, name.raw()) };
        if(location == -1) CORE_WARN(u8"Shader: Uniform '{}' not found!", name);
        uniform_location_cache_[name] = location;
        return location;
        // in glUniform functions, if location == -1, it basically won't do anything,
        // there won't be any errors, so we just call a CORE_WARN
    }

    void glGShader::compile_or_get_vulkan_spirv
        (const ShaderPaths& paths, const ShaderSources& sources)
    {
        shaderc::Compiler compiler{};
        shaderc::CompileOptions options{};
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetIncluder(std::make_unique<ShaderIncluder>());

    #ifndef RKE_DEBUG
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
    #endif

        for(uint32 stage{}; stage < paths.size(); stage++)
        {
            vulkan_spirv_[stage].clear();
            if(paths[stage].empty()) continue;

            Path cached_path{};
            Path cache_directory{ file::shader_cache_dir() / u8"vulkan" };
            try {
                if(!cache_directory.exists()) fs::create_directories(cache_directory);
                // fingerprint covers the stage source + its includes
                uint64 fingerprint{ sources[stage].fingerprint };
                CORE_ASSERT(fingerprint, u8"glGShader: Fingerprint empty!");
                cached_path = cache_directory /
                    stage_cache_file_name(name_, paths[stage], fingerprint, u8"spirv");
            } catch(const std::exception& e)
                { CORE_ASSERT(false, u8"glGShader: Exception '{}'.", e.what()); }

            std::ifstream in(cached_path.string().raw(), std::ios::in | std::ios::binary);
            if(in.is_open()) { // vulkan cache found
                in.seekg(0, std::ios::end);
                std::streampos size{ in.tellg() };
                in.seekg(0, std::ios::beg);

                std::vector<uint32>& data{ vulkan_spirv_[stage] };
                data.resize(size / sizeof(uint32));
                in.read(reinterpret_cast<char*>(data.data()), size);
            } else { // Compiling
                CORE_INFO(u8"glGShader: Compling '{}' to vulkan spirv...", paths[stage].filename());
                shaderc::SpvCompilationResult spirv{ compiler.CompileGlslToSpv
                (
                    sources[stage].content.raw(), to_shaderc(static_cast<ShaderStage>(stage)),
                    paths[stage].string().raw(), options
                )};
                if(spirv.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    auto error_msg{ spirv.GetErrorMessage() };
                    error_msg = std::regex_replace(error_msg, std::regex("\\{"), "{{");
                    error_msg = std::regex_replace(error_msg, std::regex("\\}"), "}}");
                    CORE_ASSERT(false, u8"glGShader: Vulkan SPIR-V "
                        u8"compilation failed for shader '{}':\n'{}'!", name_, error_msg);
                }

                vulkan_spirv_[stage] = std::vector<uint32>(spirv.cbegin(), spirv.cend());
                std::vector<uint32>& data{ vulkan_spirv_[stage] };
                file::write_file_binary(cached_path, data.data(), data.size() * sizeof(uint32));
                prune_orphan_caches(cache_directory, cached_path.filename());
                CORE_INFO(u8"glGShader: Vulkan spriv compiling finished.");
            }
        }
    }

    void glGShader::compile_or_get_opengl_spirv
        (const ShaderPaths& paths, const ShaderSources& sources)
    {
        shaderc::Compiler compiler{};
        shaderc::CompileOptions options{};
        options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);

        for(uint32 stage{}; stage < paths.size(); stage++)
        {
            opengl_spirv_[stage].clear();
            if(paths[stage].empty()) continue;

            Path cached_path{};
            Path cache_directory{ file::shader_cache_dir() / u8"opengl" };
            try {
                // fingerprint must match the vulkan stage, otherwise the
                // cross-compiled GLSL source would disagree with the cache
                uint64 fingerprint{ sources[stage].fingerprint };
                CORE_ASSERT(fingerprint, u8"glGShader: Fingerprint empty!");
                if(!cache_directory.exists()) fs::create_directories(cache_directory);
                cached_path = cache_directory /
                    stage_cache_file_name(name_, paths[stage], fingerprint, u8"spirv");
            } catch(std::exception& e)
                { CORE_ASSERT(false, u8"glGShader: Exception '{}'.", e.what()); }

            std::ifstream in(cached_path.string().raw(), std::ios::in | std::ios::binary);
            if(in.is_open()) {
                in.seekg(0, std::ios::end);
                std::streampos size{ in.tellg() };
                in.seekg(0, std::ios::beg);

                std::vector<uint32>& data{ opengl_spirv_[stage] };
                data.resize(size / sizeof(uint32));
                in.read(reinterpret_cast<char*>(data.data()), size);
            } else {
                CORE_INFO(u8"glGShader: Compling '{}' to OpenGL spirv...", paths[stage].filename());
                CORE_ASSERT(!vulkan_spirv_[stage].empty(), u8"glGShader: Vulkan spirv not compiled!");
                spirv_cross::CompilerGLSL glsl_compiler{ vulkan_spirv_[stage] };
                CharBuffer gl_source{ glsl_compiler.compile() };

                shaderc::SpvCompilationResult gl_spirv_res
                {
                    compiler.CompileGlslToSpv(gl_source,
                        to_shaderc(static_cast<ShaderStage>(stage)),
                        paths[stage].string().raw(), options
                    )
                };
                if(gl_spirv_res.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    auto error_msg{ gl_spirv_res.GetErrorMessage() };
                    error_msg = std::regex_replace(error_msg, std::regex("\\{"), "{{");
                    error_msg = std::regex_replace(error_msg, std::regex("\\}"), "}}");
                    CORE_ASSERT(false, u8"glGShader: OpenGL SPIR-V "
                        "compilation failed for shader '{}':\n{}!", name_, error_msg);
                }
                opengl_spirv_[stage] = std::vector<uint32>(gl_spirv_res.cbegin(), gl_spirv_res.cend());

                std::vector<uint32>& data{ opengl_spirv_[stage] };
                file::write_file_binary(cached_path, data.data(), data.size() * sizeof(uint32));
                prune_orphan_caches(cache_directory, cached_path.filename());
                CORE_INFO(u8"glGShader: OpenGL spirv compiling finished.");
            }
        }
    }

    void glGShader::create_program()
    {
        GLuint program{ glCreateProgram() };

        std::vector<GLuint> shader_ids{};
        for(uint32 stage{}; stage < opengl_spirv_.size(); stage++)
        {
            std::vector<uint32>& spirv{ opengl_spirv_[stage] };
            if(spirv.empty()) continue;

            GLenum gl_stage{ to_gl_enum(static_cast<ShaderStage>(stage)) };
            GLuint shader_id{ glCreateShader(gl_stage) };
            glShaderBinary(1, &shader_id, GL_SHADER_BINARY_FORMAT_SPIR_V,
                           spirv.data(), spirv.size() * sizeof(uint32));
            glSpecializeShader(shader_id, "main", 0, nullptr, nullptr);
            glAttachShader(program, shader_id);
            shader_ids.push_back(shader_id);
        }

        glLinkProgram(program);

    // Error Checking
        GLint is_linked{};
        glGetProgramiv(program, GL_LINK_STATUS, &is_linked);
        if(is_linked == GL_FALSE)
        {
            GLint max_length{};
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);
            std::vector<GLchar> info_log(max_length);
            glGetProgramInfoLog(program, max_length, &max_length, info_log.data());

            CORE_ERROR(u8"glGShader: Linking failed for shader \'{}\':\n{}", name_, info_log.data());

            glDeleteProgram(program);
            for(auto id : shader_ids) glDeleteShader(id);

            CORE_ASSERT(false, u8"glGShader: Failed to link programme!");
        }

        for(auto id : shader_ids)
        {
            glDetachShader(program, id);
            glDeleteShader(id);
        }
        gal_id_ = program;
    }
}
