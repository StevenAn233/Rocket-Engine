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
        friend class Scene;
        friend class SceneSerializer;

        ScriptManager(Scene* owner);
        ~ScriptManager();

        ScriptManager(const ScriptManager& other) = delete;
        ScriptManager& operator=(const ScriptManager& other) = delete;
        ScriptManager(ScriptManager&&) = delete;
        ScriptManager& operator=(ScriptManager&&) = delete;

        void clear();
        void on_update(double dt);
        void on_mouse_scrolled(float x_offset, float y_offset);

        void dispatch_contacts (
            const std::vector<Contact>& begin_contacts_solid,
            const std::vector<Contact>& end_contacts_solid,
            const std::vector<Contact>& begin_contacts_sensor,
            const std::vector<Contact>& end_contacts_sensor
        );

        void sync_all_to_cache();
        void flush_scripts();

        Script* get_script(EntityHandle handle);
    private:
        struct ScriptCache
        {
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
            for(const ScriptCache& cache : script_cache_)
                if(cache.script) dispatch_cache.push_back(cache.script.get());

            for(Script* script : dispatch_cache)
                std::invoke(std::forward<Func>(func), script);
        }

        void attach_script_to(Script* script, EntityHandle handle);
        void init_script(Script* script, EntityHandle handle);
        void bury(Scope<Script> script);

        void align_cache(); // keep cache size == storage size
        // validate slot, (re)create script
        ScriptCache* refresh_cache(EntityHandle handle, Size index, ScriptType type); 
        void contact_callback(EntityHandle lhs, EntityHandle rhs, ContactType type);

        static void on_script_com_destroy(entt::registry& reg, entt::entity ent);
    private:
        Scene* owner_;
        std::vector<ScriptCache> script_cache_{};
        std::vector<Scope<Script>> graveyard_{};
    };
}
