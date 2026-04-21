module;

// vulkan SDK
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <glad/glad.h>

module Shader;
import :OpenGL;

import Log;
import FileUtils;
import Application;

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
        CORE_ASSERT(false, u8"glShader: Unknown shader stage!");
        return 0;
    }

    static inline shaderc_shader_kind to_shaderc(GLenum stage)
    {
        switch(stage)
        {
        case GL_VERTEX_SHADER:          return shaderc_glsl_vertex_shader;
        case GL_FRAGMENT_SHADER:        return shaderc_glsl_fragment_shader;
        case GL_GEOMETRY_SHADER:        return shaderc_glsl_geometry_shader;
        case GL_COMPUTE_SHADER:         return shaderc_glsl_compute_shader;
        case GL_TESS_CONTROL_SHADER:    return shaderc_glsl_tess_control_shader;
        case GL_TESS_EVALUATION_SHADER: return shaderc_glsl_tess_evaluation_shader;
        }
        CORE_ASSERT(false, u8"glShader: Unknown shader stage!");
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
}

namespace rke
{
    glShader::glShader(const String& name, const ShaderPathMap& paths)
        : name_(std::move(name))
    {
        ShaderSources sources {};
        ShaderPaths   gl_paths{};

        for(const auto& [stage, path] : paths)
        {
            CORE_ASSERT(path.exists(), u8"glShader: "
                u8"Shader path '{}' doesn't exist!", path.string());
        
            GLenum gl_type{ to_gl_enum(stage) };
            sources [gl_type] = { path, file::read_file_string(path) };
            gl_paths[gl_type] = path;
        }

        compile_or_get_vulkan_spirv(sources );
        compile_or_get_opengl_spirv(gl_paths);
        create_program();
    }
    glShader::~glShader() {
        opengl_spirv_.clear();
        vulkan_spirv_.clear();
        glDeleteProgram(renderer_id_);
    }

    void glShader::bind  () const { glUseProgram(renderer_id_); }
    void glShader::unbind() const { glUseProgram(0); }

    const Shader& glShader::upload(StringView name, int v) const
    {
        glUniform1i(get_uniform_location(name), v);
        return *this;
    }
    const Shader& glShader::upload(StringView name, float v) const
    {
        glUniform1f(get_uniform_location(name), v);
        return *this;
    }
    const Shader& glShader::upload(StringView name, int count, int* data) const
    {
        glUniform1iv(get_uniform_location(name), count, data);
        return *this;
    }
    const Shader& glShader::upload(StringView name, float v0, float v1, float v2) const
    {
        glUniform3f(get_uniform_location(name), v0, v1, v2);
        return *this;
    }
    const Shader& glShader::upload(StringView name, glm::vec2 vec) const
    {
        glUniform2f(get_uniform_location(name), vec.x, vec.y);
        return *this;
    }
    const Shader& glShader::upload(StringView name, glm::vec3 vec) const
    {
        glUniform3f(get_uniform_location(name), vec.x, vec.y, vec.z);
        return *this;
    }
    const Shader& glShader::upload(StringView name, glm::vec4 vec) const
    {
        glUniform4f(get_uniform_location(name), vec[0], vec[1], vec[2], vec[3]);
        return *this;
    }
    const Shader& glShader::upload(StringView name,
                                   float v0, float v1, float v2, float v3) const
    {
        glUniform4f(get_uniform_location(name), v0, v1, v2, v3);
        return *this;
    }

    int glShader::get_uniform_location(const String& name) const
    {
        auto it{ uniform_location_cache_.find(name) };
        if(it != uniform_location_cache_.end()) return it->second;

        GLint location{ glGetUniformLocation(renderer_id_, name.raw()) };
        if(location == -1) CORE_WARN(u8"Shader: Uniform '{}' not found!", name);
        uniform_location_cache_[name] = location;
        return location;
        // in glUniform functions, if location == -1, it basically won't do anything,
        // there won't be any errors, so we just call a CORE_WARN
    }

    void glShader::compile_or_get_vulkan_spirv(const ShaderSources& sources)
    {
        shaderc::Compiler compiler{};
        shaderc::CompileOptions options{};
        options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_2);
        options.SetIncluder(std::make_unique<ShaderIncluder>());

    #ifndef RKE_DEBUG
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
    #endif

        vulkan_spirv_.clear();
        for(auto&& [stage, source] : sources)
        {
            Path cached_path{};
            try {
                const Path& assets_dir{ Application::get().get_engine_assets_dir() };
                Path cache_directory{ file::get_shader_cache_dir() / u8"vulkan" };
                if(!cache_directory.exists()) fs::create_directories(cache_directory);
                cached_path = cache_directory /
                    (source.first.filename().string() + u8".cached_vulkan");
            } catch(const std::exception& e)
                { CORE_ASSERT(false, u8"glShader: Exception '{}'.", e.what()); }

            std::ifstream in(cached_path.string().raw(), std::ios::in | std::ios::binary);
            if(in.is_open()) { // vulkan cache found
                in.seekg(0, std::ios::end);
                std::streampos size{ in.tellg() };
                in.seekg(0, std::ios::beg);

                std::vector<uint32>& data{ vulkan_spirv_[stage] }; // pushed vector into map
                data.resize(size / sizeof(uint32));
                in.read(reinterpret_cast<char*>(data.data()), size);
            } else { // Compiling
                CORE_INFO(u8"glShader: Compling '{}' to vulkan spirv...", source.first.filename());
                shaderc::SpvCompilationResult spirv {
                    compiler.CompileGlslToSpv(source.second.raw(), to_shaderc(stage),
                                              source.first.string().raw(), options)
                };
                if(spirv.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    auto error_msg{ spirv.GetErrorMessage() };
                    error_msg = std::regex_replace(error_msg, std::regex("\\{"), "{{");
                    error_msg = std::regex_replace(error_msg, std::regex("\\}"), "}}");
                    CORE_ASSERT(false, u8"glShader: Vulkan SPIR-V "
                        u8"compilation failed for shader '{}':\n'{}'!", name_, error_msg);
                }

                vulkan_spirv_[stage] = std::vector<uint32>(spirv.cbegin(), spirv.cend());
                std::vector<uint32>& data{ vulkan_spirv_[stage] };
                file::write_file_binary(cached_path, data.data(), data.size() * sizeof(uint32));
                CORE_INFO(u8"glShader: Vulkan spriv compiling finished.");
            }
        }
    }

    void glShader::compile_or_get_opengl_spirv(const ShaderPaths& paths)
    {
        shaderc::Compiler compiler{};
        shaderc::CompileOptions options{};
        options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);

        opengl_spirv_.clear();
        for(auto&& [stage, spirv] : vulkan_spirv_)
        {
            Path cached_path{}, original_path{};
            try {
                original_path = paths.at(stage);
                const Path& assets_dir{ Application::get().get_engine_assets_dir() };
                Path cache_directory{ file::get_shader_cache_dir() / u8"opengl" };
                if(!cache_directory.exists()) fs::create_directories(cache_directory);
                cached_path = cache_directory /
                    (original_path.filename().string() + u8".cached_opengl");
            } catch(std::exception& e)
                { CORE_ASSERT(false, u8"glShader: Exception '{}'.", e.what()); }

            std::ifstream in(cached_path.string().raw(), std::ios::in | std::ios::binary);
            if(in.is_open()) {
                in.seekg(0, std::ios::end);
                std::streampos size{ in.tellg() };
                in.seekg(0, std::ios::beg);

                std::vector<uint32>& data{ opengl_spirv_[stage] }; // pushed vector into map
                data.resize(size / sizeof(uint32));
                in.read(reinterpret_cast<char*>(data.data()), size);
            } else {
                CORE_INFO(u8"glShader: Compling '{}' to OpenGL spirv...",
                    original_path.filename());
                spirv_cross::CompilerGLSL glsl_compiler(spirv);
                CharBuffer gl_source{ glsl_compiler.compile() };

                shaderc::SpvCompilationResult gl_spirv {
                    compiler.CompileGlslToSpv(gl_source,
                        to_shaderc(stage), original_path.string().raw(), options)
                };
                if(gl_spirv.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    auto error_msg{ gl_spirv.GetErrorMessage() };
                    error_msg = std::regex_replace(error_msg, std::regex("\\{"), "{{");
                    error_msg = std::regex_replace(error_msg, std::regex("\\}"), "}}");
                    CORE_ASSERT(false, u8"glShader: OpenGL SPIR-V "
                        "compilation failed for shader '{}':\n{}!", name_, error_msg);
                }

                opengl_spirv_[stage] = std::vector<uint32>(gl_spirv.cbegin(), gl_spirv.cend());

                std::vector<uint32>& data{ opengl_spirv_[stage] };
                file::write_file_binary(cached_path, data.data(), data.size() * sizeof(uint32));
                CORE_INFO(u8"glShader: OpenGL spirv compiling finished.");
            }
        }
    }

    void glShader::create_program()
    {
        GLuint program{ glCreateProgram() };

        std::vector<GLuint> shader_ids{};
        shader_ids.reserve(opengl_spirv_.size());
        for(auto& [stage, spirv] : opengl_spirv_)
        {
            GLuint shader_id{ glCreateShader(stage) };
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

            CORE_ERROR(u8"glShader: Linking failed for shader \'{}\':\n{}", name_, info_log.data());

            glDeleteProgram(program);
            for(auto id : shader_ids) glDeleteShader(id);

            CORE_ASSERT(false, u8"glShader: Failed to link programme!");
        }
    // ---

        for(auto id : shader_ids) {
            glDetachShader(program, id);
            glDeleteShader(id);
        }
        renderer_id_ = program;
    }
}
