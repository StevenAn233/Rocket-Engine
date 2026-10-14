module;
export module FXAAEffect;

import rke;

export namespace rke
{
    class FXAAEffect : public PostProcessEffect
    {
    public:
        struct Uniforms { glm::vec2 inverse_viewport_size{}; };

        FXAAEffect(String name, std::function<bool()> func = nullptr);
        
        Category get_category() const override { return Category::System; }
    private:
        bool apply(const GTexture2D* source, FrameBuffer* destination) override;
        void on_viewport_resized(uint32 w, uint32 h) override;
    private:
        Ref<UniformBuffer> ubo_{};
    };
}
