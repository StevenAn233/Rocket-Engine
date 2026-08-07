module;
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

        OutlineEffect(String name, std::function<bool()> func = nullptr);

        Category get_category() const override { return Category::Helper; }
        bool apply(const Texture2D* source, FrameBuffer* destination) override;
        void serialize_to(ConfigWriter& writer) const override;
        void deserialize_from(const ConfigReader& reader) override;

        void set_target	(Entity target );
        void set_samples(uint32 samples);

        void set_color(glm::vec4 color);
        void set_thickness(float thickness);
        inline glm::vec4 get_color() const { return uniforms_.outline_color; }
        inline float get_thickness() const { return uniforms_.thickness; }
    private:
        void on_viewport_resized(uint32 w, uint32 h) override;
    private:
        Ref<FrameBuffer> outline_fbo_{};
        Uniforms uniforms_{};
        Entity target_{};
    };
}
