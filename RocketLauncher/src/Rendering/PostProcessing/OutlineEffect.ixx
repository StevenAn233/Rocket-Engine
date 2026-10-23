module;
export module OutlineEffect;

import rke;

export namespace rke
{
    class OutlineEffect : public PostProcessEffect
    {
    public:
        using Target = std::pair<const Mesh*, glm::mat4>;
        using TargetGetter = std::function<std::vector<Target>()>;
        using CameraGetter = std::function<glm::mat4()>;

        struct Uniforms{ glm::vec4 outline_color{}; }; // srgb

        struct Context
        {
            TargetGetter target_getter{ nullptr };
            CameraGetter camera_getter{ nullptr };

            Context() = default;
            Context(TargetGetter tgetter, CameraGetter cgetter)
                : target_getter(std::move(tgetter))
                , camera_getter(std::move(cgetter)) {}

            Context(const Context& other)
                : target_getter(other.target_getter)
                , camera_getter(other.camera_getter) {}
            
            Context& operator=(const Context& other)
            {
                target_getter = other.target_getter;
                camera_getter = other.camera_getter;
                return *this;
            }
            
            Context(Context&& other)
                : target_getter(std::move(other.target_getter))
                , camera_getter(std::move(other.camera_getter)) {}
            
            Context& operator=(Context&& other)
            {
                target_getter = std::move(other.target_getter);
                camera_getter = std::move(other.camera_getter);
                return *this;
            }
        };

        OutlineEffect(String name, std::function<bool()> extra = nullptr);
        ~OutlineEffect() override {}
        Category get_category() const override { return Category::Helper; }

        bool apply(const GTexture2D* source, FrameBuffer* destination) override;

        inline void set_context(Context context) { context_ = std::move(context); }
        void set_color(glm::vec4 color);
        void set_thickness(float thickness);
        inline glm::vec4 get_color() const { return uniforms_.outline_color; }
        inline float get_thickness() const { return thickness_; }

        inline void set_wireframe(bool on) { wireframe_ = on; }
        inline bool is_wireframe() const { return wireframe_; }

        inline uint32 get_samples() const { return samples_; }
        uint8 get_samples_shift() const;
        void set_samples(uint8 shift);

        static void refresh_shaders()
        {
            get_outline_shader().clear_gshaders();
            get_copy_shader().clear_gshaders();
        }
    private:
        void on_serialize(ConfigWriter& writer) const override;
        void on_deserialize(const ConfigReader& reader) override;

        void on_imgui_render() override;
        void on_viewport_resized(uint32 w, uint32 h) override;

        void ensure_buffers(Size edge_capacity);
        uint32 build_outline(const std::vector<Target>& targets,
            const glm::mat4& view_proj, glm::vec2 viewport_size);

        static Shader& get_outline_shader();
        static Shader& get_copy_shader();
    private:
        Context context_{};

        Uniforms uniforms_{};
        Ref<UniformBuffer> ubo_{};

        float thickness_{ 1.0f }; // pixels
        bool wireframe_{ false }; // draw every triangle edge if true

        uint32 samples_{ 8 };
        Scope<FrameBuffer> outline_fbo_{};

        Scope<VertexArray> vao_{};
        Ref<VertexBuffer > vbo_{};
        Ref<IndexBuffer  > ibo_{};
        Size edge_capacity_{}; // how many edges the current buffers can contain
    };
}
