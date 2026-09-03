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
    class PostProcessor
    {
    public:
        PostProcessor(glm::vec4 clear_color);
        PostProcessor(const PostProcessor&) = delete;
        PostProcessor(PostProcessor&&) = delete;

        PostProcessEffect* push_effect(Scope<PostProcessEffect> effect);
        Scope<PostProcessEffect> pop_effect();
        PostProcessEffect* get_effect(const String& name); // expensive
        void refresh_all_effect_shaders();

        const GTexture2D* process(const GTexture2D* source);

        void on_viewport_resized(uint32 w, uint32 h);
        void clean_up() { fbos_[0]->clear(); fbos_[1]->clear(); }
    private:
        Scope<FrameBuffer> fbos_[2]{};
        uint32 viewport_w_{}, viewport_h_{};

        std::vector<Scope<PostProcessEffect>> effects_stack_{}; // sequential
        ToneMapping tone_mapping_{ u8"Tone Mapping" };
    };
}
