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

        struct Uniforms{ glm::vec4 outline_color{}; };

        OutlineEffect(String name, Window* context,
            std::function<bool()> enabled_situation,
            TargetGetter target_getter, CameraGetter camera_getter);

        Category get_category() const override { return Category::Helper; }
        
        void set_color(glm::vec4 color);
        void set_thickness(float thickness);
        inline glm::vec4 get_color() const { return uniforms_.outline_color; }
        inline float get_thickness() const { return thickness_; }

        inline void set_wireframe(bool on) { wireframe_ = on; }
        inline bool is_wireframe() const { return wireframe_; }
    private:
        bool apply(const GTexture2D* source, FrameBuffer* destination) override;

        void refresh_shader() override;
        void serialize_to(ConfigWriter& writer) const override;
        void deserialize_from(const ConfigReader& reader) override;
        void on_imgui_render() override;

        void ensure_buffers(Size edge_capacity);
        uint32 build_outline(const std::vector<Target>& targets,
            const glm::mat4& view_proj, glm::vec2 viewport_size);
    private:
        Window* context_;
        TargetGetter target_getter_;
        CameraGetter camera_getter_;

        Uniforms uniforms_{};
        Ref<UniformBuffer> ubo_{};

        Scope<Shader> outline_shader_{};
        Scope<Shader> copy_shader_{};

        float thickness_{ 1.0f }; // pixels
        bool wireframe_{ false }; // draw every triangle edge if true

        Scope<VertexArray> vao_{};
        Ref<VertexBuffer > vbo_{};
        Ref<IndexBuffer  > ibo_{};
        Size edge_capacity_{}; // how many edges the current buffers can contain
    };
}
