module;
module PostProcessor;

import Log;
import RenderCommand;
import Application;

namespace rke
{
    PostProcessor::PostProcessor(glm::vec4 clear_color)
    {
        FrameBuffer::Specification spec
            { .attachment_spec{{ GTexture::Format::RGBA16F, clear_color }}};
        fbos_[0] = FrameBuffer::create(spec);
        fbos_[1] = FrameBuffer::create(spec);

        tone_mapping_.set_uniform({});
    }

    PostProcessEffect* PostProcessor::push_effect(Scope<PostProcessEffect> effect)
    {
        effects_stack_.push_back(std::move(effect));
        return effects_stack_.back().get();
    }

    Scope<PostProcessEffect> PostProcessor::pop_effect()
    {
        Scope<PostProcessEffect> effect{ std::move(effects_stack_.back()) };
        effects_stack_.pop_back();
        return effect;
    }

    PostProcessEffect* PostProcessor::get_effect(const String& name)
    {
        for(const auto& effect : effects_stack_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            if(effect->get_name() == name) return effect.get();
        }
        CORE_WARN(u8"PostProcessor: Effect not found!");
        return nullptr;
    }

    void PostProcessor::refresh_all_effect_shaders()
    {
        for(const auto& effect : effects_stack_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            effect->refresh_shader();
        }
    }

    const GTexture2D* PostProcessor::process(const GTexture2D* source)
    {
        if(!source) return nullptr;

        clean_up();
        app().render_command().disable_blend();
        app().render_command().disable_depth_test();

        const GTexture2D* ping_pong{ source };
        uint32 fbo_index{ 0 };
        for(auto& effect : effects_stack_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            if(!effect->enabled()) continue;
            if(effect->apply(ping_pong, fbos_[fbo_index].get()))
            {
                ping_pong = fbos_[fbo_index]->get_gtexture_attached();
                fbo_index ^= 1u;
            }
        }
        CORE_ASSERT(tone_mapping_.apply(ping_pong, fbos_[fbo_index].get()),
            u8"PostProcessor: Tone mapping failed!");
        
        app().render_command().enable_depth_test();
        app().render_command().enable_blend();

        return fbos_[fbo_index]->get_gtexture_attached();
    }

    void PostProcessor::on_viewport_resized(uint32 w, uint32 h)
    {
        fbos_[0]->resize(w, h);
        fbos_[1]->resize(w, h);
        for(auto& effect : effects_stack_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            effect->on_viewport_resized(w, h);
        }
    }
}
