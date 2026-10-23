module;
export module FXAAEffect;

import rke;

export namespace rke
{
    class FXAAEffect : public PostProcessEffect
    {
    public:
        struct Uniforms { glm::vec2 inverse_viewport_size{}; };

        FXAAEffect(String name, std::function<bool()> extra = nullptr);
        ~FXAAEffect() override {}
        Category get_category() const override { return Category::System; }

        bool apply(const GTexture2D* source, FrameBuffer* destination) override;
        static void refresh_shader() { get_shader().clear_gshaders(); }
    private:
        void on_viewport_resized(uint32 w, uint32 h) override;
        static Shader& get_shader();
    private:
        Ref<UniformBuffer> ubo_{};
    };
}
