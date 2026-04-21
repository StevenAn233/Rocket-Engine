module;

#include <unordered_map>
#include <glm/glm.hpp>

export module PostProcessor;

import Types;
import String;
import HeapManager;
import Texture;
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
        PostProcessor(const PostProcessor& ) = delete;
        PostProcessor(PostProcessor&& ____ ) = delete;

        void add_effect(Scope<PostProcessEffect> effect);
        PostProcessEffect* get_effect(const String& name);
        const EffectMap& get_all_effects() const { return effects_; }

        const Texture2D* process(const Texture2D* source);

        void on_viewport_resized(uint32 w, uint32 h);
        void clean_up() { fbos_[0]->clear(); fbos_[1]->clear(); }
    private:
        Ref<FrameBuffer> fbos_[2]{};
        uint32 viewport_w_{}, viewport_h_{};

        EffectMap effects_{};
        ToneMapping tone_mapping_{ u8"Tone Mapping" };
    };
}
