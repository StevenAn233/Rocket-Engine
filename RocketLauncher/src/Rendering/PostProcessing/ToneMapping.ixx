module;
export module ToneMapping;

import rke;

export namespace rke
{
    class ToneMapping : public PostProcessEffect
    {
    public:
        struct Uniforms
        {
            float exposure{ 1.0f };
            float gamma	  { 2.2f };
        };

        ToneMapping();
        ~ToneMapping() override {}

        bool apply(const GTexture2D* source, FrameBuffer* destination) override;
        void on_imgui_render() override;

        void set_exposure(float exposure);
        void set_gamma(float gamma);

        static void refresh_shader() { get_shader().clear_gshaders(); }

        static const char8_t* get_static_type_name();
        const char8_t* get_type_name() const override;
    private:
        void on_serialize(ConfigWriter& writer) const override;
        void on_deserialize(const ConfigReader& reader) override;

        static Shader& get_shader();
    private:
        Uniforms uniforms_{};
        Ref<UniformBuffer> ubo_{};
    };
}
