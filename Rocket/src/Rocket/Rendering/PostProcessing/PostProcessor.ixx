module;

#include <unordered_map>
#include <glm/glm.hpp>

export module PostProcessor;

import Types;
import String;
import HeapManager;
import GTexture;
import FrameBuffer;
import PostProcessEffect;
import ToneMapping;

export namespace rke
{
    using EffectMap = std::unordered_map<String, Scope<PostProcessEffect>>;

    class PostProcessor
    {
    public:
        PostProcessor(glm::vec4 clear_color);
        PostProcessor(const PostProcessor&) = delete;
        PostProcessor(PostProcessor&&) = delete;

        void add_effect(Scope<PostProcessEffect> effect);
        PostProcessEffect* get_effect(const String& name);
        void refresh_all_effect_shaders();
        inline const EffectMap& get_all_effects() const { return effects_; }

        const GTexture2D* process(const GTexture2D* source);

        void on_viewport_resized(uint32 w, uint32 h);
        void clean_up() { fbos_[0]->clear(); fbos_[1]->clear(); }
    private:
        Scope<FrameBuffer> fbos_[2]{};
        uint32 viewport_w_{}, viewport_h_{};

        EffectMap effects_{};
        ToneMapping tone_mapping_{ u8"Tone Mapping" };
    };
}
