module;
module PostProcessor;

import Log;
import RenderCommand;

namespace rke
{
    PostProcessor::PostProcessor(glm::vec4 clear_color)
    {
        FrameBuffer::Specification spec
            { .attachment_spec{{ Texture::Format::RGBA16F, clear_color }}};
        fbos_[0] = FrameBuffer::create(spec);
        fbos_[1] = FrameBuffer::create(spec);

        tone_mapping_.set_uniform({});
    }

    void PostProcessor::add_effect(Scope<PostProcessEffect> effect)
        { effects_[effect->get_name()] = std::move(effect); }

    PostProcessEffect* PostProcessor::get_effect(const String& name)
    {
        if(effects_.count(name)) return effects_[name].get();
        CORE_ERROR(u8"PostProcessor: Effect not found!");
        return nullptr;
    }

    const Texture2D* PostProcessor::process(const Texture2D* source)
    {
        if(!source) return nullptr;

        std::vector<PostProcessEffect*> active_effects{};
        for(const auto& [_, effect] : effects_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            if(effect->enabled()) active_effects.push_back(effect.get());
        }

        clean_up();
        RenderCommand::disable_blend();
        RenderCommand::disable_depth_test();

        const Texture2D* ping_pong{ source };
        uint32 fbo_index{ 0 };
        for(Size i{}; i < active_effects.size(); i++) {
            if(active_effects[i]->apply(ping_pong, fbos_[fbo_index].get()))
            {
                ping_pong = fbos_[fbo_index]->get_texture();
                fbo_index ^= 1u;
            }
        }

        CORE_ASSERT(tone_mapping_.apply(ping_pong, fbos_[fbo_index].get()),
            u8"PostProcessor: Tone mapping failed!");

        RenderCommand::enable_depth_test(); // default
        RenderCommand::enable_blend();		// default

        return fbos_[fbo_index]->get_texture();
    }

    void PostProcessor::on_viewport_resized(uint32 w, uint32 h)
    {
        fbos_[0]->resize(w, h);
        fbos_[1]->resize(w, h);
    }
}
