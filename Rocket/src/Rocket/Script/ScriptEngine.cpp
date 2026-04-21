module;
module ScriptEngine;

import Log;
import Script;
import HeapManager;
import Components;
import ScriptRegistry;

namespace {
    using namespace rke;
    using ScriptInstances = std::unordered_map<uint32, Scope<Script>>;

    static Scene* s_context{};
    static ScriptInstances s_instances{};

    static void on_script_component_destroyed(entt::registry&, entt::entity e)
    {
        if(!s_context || !s_context->in_runtime()) return;
        ScriptEngine::destroy_script(static_cast<uint32>(e));
    }
}

namespace rke
{
    void ScriptEngine::on_runtime_start(Scene* scene)
    {
        s_context = scene;
        if(!s_context) {
            CORE_ERROR(u8"ScriptEngine: Scene(context) empty!");
            return;
        }
        auto& registry{ *s_context->registry_ };

        registry.on_destroy<NativeScriptComponent>()
            .connect<&on_script_component_destroyed>();

        auto view{ registry.view<NativeScriptComponent>() };
        for(auto entt : view)
        {
            Entity entity{ s_context->get_entity(entt) };
            const auto& name{ entity.get<NativeScriptComponent>().script_name };
            if(!name.empty()) create_script(entity.get_handle(), name);
        }
    }

    void ScriptEngine::on_runtime_stop()
    {
        for(auto& [_, script] : s_instances)
            { if(script) script->on_destroy(); }
        s_instances.clear();
        s_context = nullptr;
    }

    void ScriptEngine::on_update(float dt)
    {
        if(!s_context) {
            CORE_ERROR(u8"ScriptEngine: Scene(context) empty!");
            return;
        }
        for(auto& [handle, script] : s_instances)
        {
            Entity entity{ s_context->get_entity(handle) };
            if(entity.has<NativeScriptComponent>())
            {
                bool to_update{ entity.get<NativeScriptComponent>().wants_to_update };
                if(script && to_update) script->on_update(dt);
            }
        }
    }

    void ScriptEngine::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        for(auto& [_, script] : s_instances)
            script->on_mouse_scrolled(e);
    }

    void ScriptEngine::update_script(Entity entity)
    {
        if(!s_context || !s_context->in_runtime()) return;
        destroy_script(entity.get_handle());
        if(entity.has<NativeScriptComponent>())
        {
            const auto& name{ entity.get<NativeScriptComponent>().script_name };
            if(!name.empty()) create_script(entity.get_handle(), name);
        }
    }

    void ScriptEngine::create_script(uint32 handle, const String& name)
    {
        CORE_ASSERT(s_context && s_context->in_runtime(),
            u8"ScriptEngine: Can only create script within runtime!");
        auto script{ ScriptRegistry::construct_through_name(name) };
        if(script) {
            script->context_ = s_context->get_entity(handle);
            script->on_create();
            s_instances[handle] = std::move(script);
        }
    }

    void ScriptEngine::destroy_script(uint32 handle)
    {
        if(s_instances.empty()) return; // important
        if(s_instances.find(handle) != s_instances.end())
        {
            s_instances[handle]->on_destroy();
            s_instances.erase(handle);
        }
    }
}
