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

        bool apply(const Texture2D* source, FrameBuffer* destination) override;
        Category get_category() const override { return Category::System; }
        void set_uniform(const Uniforms& uniforms);
    };
}
