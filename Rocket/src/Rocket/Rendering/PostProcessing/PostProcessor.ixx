module;

#include <vector>
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
import RkDylib;

export namespace rke
{
    class RKE_API PostProcessor
    {
    public:
        PostProcessor();
        ~PostProcessor() = default;
        
        PostProcessor(const PostProcessor&) = delete;
        PostProcessor& operator=(const PostProcessor&) = delete;
        PostProcessor(PostProcessor&&) = delete;
        PostProcessor& operator=(PostProcessor&&) = delete;

        inline void clear() { effects_stack_.clear(); }
        inline bool empty() const { return effects_stack_.empty(); }
        
        PostProcessEffect* push_effect(Scope<PostProcessEffect> effect);
        Scope<PostProcessEffect> pop_effect();
        // swap/sort effect...
        
        const GTexture2D* process(const GTexture2D* source);

        void serialize_to(ConfigWriter& writer, const RkDylib& dylib) const;
        void deserialize_from(const ConfigReader& reader, const RkDylib& dylib);

        void on_imgui_render(uint32 tree_flags = 0);
        void on_viewport_resized(uint32 w, uint32 h);
    private:
        Scope<FrameBuffer> fbos_[2]{};
        uint32 viewport_w_{}, viewport_h_{};
        std::vector<Scope<PostProcessEffect>> effects_stack_{}; // sequential
    };
}
