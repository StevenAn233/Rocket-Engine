export module OutlineEffect;

import rke;

export namespace rke
{
    class OutlineEffect : public PostProcessEffect
    {
    public:
        struct Uniforms
        {
            alignas(16) glm::vec4 outline_color{};
            alignas(16) float thickness{ 1.0f };
        };

        OutlineEffect(String name);

        Category get_category() const override { return Category::Helper; }
        bool apply(const Texture2D* source, FrameBuffer* destination) override;
        void on_imgui_render() override;
        void serialize_to(ConfigWriter* writer) const override;
        void deserialize_from(const ConfigReader* reader) override;

        void on_viewport_resized(uint32 w, uint32 h);
        void set_target	(Entity target );
        void set_samples(uint32 samples);

        void set_color(glm::vec4 color);
        void set_thickness(float thickness);
    private:
        Ref<FrameBuffer> outline_fbo_{}; // has its ownership
        Uniforms uniforms_{};
        Entity target_{};
    };
}
