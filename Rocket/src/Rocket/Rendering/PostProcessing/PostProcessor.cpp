module;
module PostProcessor;

import Log;
import Layout;
import RenderCommand;
import Application;

namespace rke
{
    PostProcessor::PostProcessor(String name, glm::vec4 clear_color)
        : name_(std::move(name))
    {
        FrameBuffer::Specification spec
            { .attachment_spec{{ GTexture::Format::RGBA16F, clear_color }}};
        fbos_[0] = FrameBuffer::create(spec);
        fbos_[1] = FrameBuffer::create(spec);
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
            if(!effect->is_enabled()) continue;
            if(effect->apply(ping_pong, fbos_[fbo_index].get()))
            {
                ping_pong = fbos_[fbo_index]->get_gtexture_attached();
                fbo_index ^= 1u;
            }
        }
        app().render_command().enable_depth_test();
        app().render_command().enable_blend();
        return ping_pong;
    }

    void PostProcessor::refresh_all_shaders()
    {
        // to be impl... or remove
    }

    void PostProcessor::serialize_all_to(ConfigWriter& writer) const
    {
        writer.begin_map(name_);
        for(const auto& effect : effects_stack_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            writer.begin_map(effect->get_name());
            effect->serialize_to(writer);
            writer.end_map();
        }
        writer.end_map();
    }

    void PostProcessor::deserialize_all_from(const ConfigReader& reader)
    {
        auto all_effects{ reader.get_child(name_) };
        if(!all_effects || !all_effects->is_map())
        {
            CORE_ERROR(u8"PostProcessir: '{}' not found!", name_);
            return;
        }
        for(const auto& effect : effects_stack_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            auto config{ all_effects->get_child(effect->get_name()) };
            if(!config || !config->is_map())
            {
                CORE_ERROR(u8"PostProcessor: "
                    u8"Effect '{}' not found!", effect->get_name());
                continue;
            }
            effect->deserialize_from(*config);
        }
    }

    void PostProcessor::on_imgui_render(uint32 tree_flags)
    {
        for(Size i{}; i < effects_stack_.size(); i++)
        {
            Scope<PostProcessEffect>& effect{ effects_stack_[i] };
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            if(layout::begin_tree_node_branch_impl
                (effect->get_name(), tree_flags, std::bit_cast<void*>(i)))
            {
                effect->on_imgui_render();
                layout::end_tree_node_branch_impl();
            }
        }
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
