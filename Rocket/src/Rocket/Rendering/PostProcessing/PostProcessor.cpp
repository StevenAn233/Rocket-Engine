module;
module PostProcessor;

import Log;
import Layout;
import RenderCommand;
import Application;
import ConfigProxy;
import EffectAccess;

namespace rke
{
    PostProcessor::PostProcessor()
    {
        FrameBuffer::Specification spec
        {
            .attachment_spec {{
                GTexture::Format::RGBA16F,
                glm::vec4(0.0f)
            }}
        };
        fbos_[0] = FrameBuffer::create(spec);
        fbos_[1] = FrameBuffer::create(spec);
        CORE_ASSERT(fbos_[0] && fbos_[1],
            u8"PostProcessor: Failed to create fbos!");
    }

    PostProcessEffect* PostProcessor::push_effect(Scope<PostProcessEffect> effect)
    {
        if(!effect) return nullptr;
        if(viewport_w_ && viewport_h_)
            effect->on_viewport_resized(viewport_w_, viewport_h_);

        effects_stack_.push_back(std::move(effect));
        return effects_stack_.back().get();
    }

    Scope<PostProcessEffect> PostProcessor::pop_effect()
    {
        if(effects_stack_.empty()) return nullptr;
        Scope<PostProcessEffect> effect{ std::move(effects_stack_.back()) };
        effects_stack_.pop_back();
        return effect;
    }

    const GTexture2D* PostProcessor::process(const GTexture2D* source)
    {
        if(!source) return nullptr;
        const GTexture2D* ping_pong{ source };

        app().render_command().disable_blend();
        app().render_command().disable_depth_test();
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

    void PostProcessor::serialize_to(ConfigWriter& writer, const RkDylib& dylib) const
    {
        writer.begin_array(u8"Post-Process");
        for(const auto& effect : effects_stack_)
        {
            if(!effect) continue;
            if(!dylib.has_effect_type(effect->get_effect_type()))
            {
                CORE_WARN(u8"PostProcessor: Effect '{}' "
                    u8"isn't registered in dylib!", effect->get_name());
                continue;
            }

            writer.begin_map();
            writer.write(u8"Type", String(effect->get_type_name()));
            writer.write(u8"Name", String(effect->get_name()));
            effect->serialize_to(writer); // <name>: { Enabled: ..., ... }
            writer.end_map();
        }
        writer.end_array();
    }

    void PostProcessor::deserialize_from(const ConfigReader& reader, const RkDylib& dylib)
    {
        clear(); // may modify
        Scope<ConfigReader> entries{ reader.get_child(u8"Post-Process") };
        if(!entries) {
            CORE_WARN(u8"PostProcessor: No 'Post-Process' entry, "
                u8"the stack is left untouched.");
            return;
        }
        if(!entries->is_array()) {
            CORE_WARN(u8"PostProcessor: 'Post-Process' isn't a sequence, "
                u8"the stack is left untouched.");
            return;
        }

        std::vector<Scope<PostProcessEffect>> loaded{};
        for(Size i{}; i < entries->size(); i++)
        {
            Scope<ConfigReader> entry{ entries->get_child(i) };
            if(!entry || !entry->is_map()) continue;

            const String type_name{ entry->get_at(u8"Type", String{}) };
            const EffectType type{ dylib.get_effect_type(type_name) };
            if(type == effect_type_null)
            {
                CORE_WARN(u8"PostProcessor: Effect type '{}' "
                    u8"isn't registered!", type_name);
                continue;
            }
            
            String name{ entry->get_at(u8"Name", String{}) };
            if(name.empty())
            {
                CORE_WARN(u8"PostProcessor: Effect name empty!");
                continue;
            }

            Scope<PostProcessEffect> effect{ dylib.construct_effect(type, name) };
            if(!effect) {
                CORE_ERROR(u8"PostProcessor: "
                    u8"Failed to construct effect '{}'!", type_name);
                continue;
            }
            effect->deserialize_from(*entry);
            loaded.push_back(std::move(effect));
        }

        effects_stack_ = std::move(loaded);
        if(viewport_w_ && viewport_h_)
            on_viewport_resized(viewport_w_, viewport_h_);
    }

    void PostProcessor::on_imgui_render(uint32 tree_flags)
    {
        for(uintptr i{}; i < effects_stack_.size(); i++)
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
        viewport_w_ = w;
        viewport_h_ = h;
        fbos_[0]->resize(w, h);
        fbos_[1]->resize(w, h);
        for(auto& effect : effects_stack_)
        {
            CORE_ASSERT(effect, u8"PostProcessor: Effect Empty!");
            effect->on_viewport_resized(w, h);
        }
    }
}
