module;

#include <memory>
#include <vector>
#include <unordered_map>
#include <entt/entt.hpp>
namespace rke { class Scene; }

export module ScriptManager;

import Types;
import Script;
import HeapManager;
import EntityAccess;
import ScriptAccess;
import Components;

export namespace rke
{
    class ScriptManager
    {
    public:
        friend class SceneHierarchyPanel; // may modify

        ScriptManager(Scene* owner);
        ~ScriptManager() = default;

        ScriptManager(const ScriptManager&) = delete;
        ScriptManager& operator=(const ScriptManager&) = delete;
        ScriptManager(ScriptManager&&) = default;
        ScriptManager& operator=(ScriptManager&&) = default;

        void on_runtime_start();
        void on_runtime_stop ();
        void on_update(double dt);
        void on_mouse_scrolled(float x_offset, float y_offset);

        void dispatch_contacts (
            const std::vector<Contact>& begin_contacts_solid,
            const std::vector<Contact>& end_contacts_solid,
            const std::vector<Contact>& begin_contacts_sensor,
            const std::vector<Contact>& end_contacts_sensor
        );

        // Destroys every script that is currently waiting in the graveyard.
        // Callers MUST do this before the dylib that created those scripts
        // gets unloaded, otherwise their vtables dangle.
        void flush_scripts();
    private:
        struct RuntimeCache
        {
            EntityHandle owner{ entity_handle_null }; // slot identity (self-healing)
            ScriptType script_type{ script_type_null };
            Scope<Script> script{};
        };

        enum class ContactType
        {
            SolidBegin,
            SolidEnd,
            SensorBegin,
            SensorEnd,
        };

        template<typename Func>
        requires std::invocable<Func, Script*>
        void for_each_script(Func&& func)
        {
            std::vector<Script*> dispatch_cache{};
            dispatch_cache.reserve(script_cache_.size());
            for(const RuntimeCache& cache : script_cache_)
            if(cache.script) dispatch_cache.push_back(cache.script.get());

            for(Script* script : dispatch_cache)
                std::invoke(std::forward<Func>(func), script);
        }

        Scope<Script> create_script(ScriptType type, EntityHandle handle);
        void destroy_script(Scope<Script> script);

        void align_cache(); // keep cache size == storage size
        // validate slot, (re)create script
        RuntimeCache* refresh_cache(EntityHandle handle, ScriptType type, Size index); 
        void sync_all_to_cache();

        void contact_callback(EntityHandle lhs, EntityHandle rhs, ContactType type);

        Script* get_script(EntityHandle handle); // For SceneHierarchy

        static void on_script_com_destroy(entt::registry& reg, entt::entity ent);
    private:
        Scene* owner_;
        std::vector<RuntimeCache> script_cache_{};
        std::vector<Scope<Script>> graveyard_{};
    };
}
