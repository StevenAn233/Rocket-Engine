module;

#include <unordered_map>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module PostProcessor;

import Types;
import String;
import HeapManager;
import GTexture;
import FrameBuffer;
import PostProcessEffect;
import ConfigProxy;

export namespace rke
{
    class RKE_API PostProcessor
    {
    public:
        friend class SceneRenderer;

        PostProcessor(String name, glm::vec4 clear_color);
        PostProcessor(const PostProcessor&) = delete;
        PostProcessor(PostProcessor&&) = delete;

        inline const String& get_name() const { return name_; }

        PostProcessEffect* push_effect(Scope<PostProcessEffect> effect);
        Scope<PostProcessEffect> pop_effect();
        // swap/sort effect...
        
        const GTexture2D* process(const GTexture2D* source);
        inline void clean_up() { fbos_[0]->clear(); fbos_[1]->clear(); }

        void serialize_all_to(ConfigWriter& writer) const;
        void deserialize_all_from(const ConfigReader& reader);
        void on_imgui_render(uint32 tree_flags = 0);
        void on_viewport_resized(uint32 w, uint32 h);
    private:
        String name_;
        Scope<FrameBuffer> fbos_[2]{};
        uint32 viewport_w_{}, viewport_h_{};
        std::vector<Scope<PostProcessEffect>> effects_stack_{}; // sequential
    };
}
