module;

#include <memory>
#include <vector>
#include <unordered_map>
#include <entt/entt.hpp>
namespace rke { class Scene; }

export module ScriptManager;

import Script;
import Types;
import HeapManager;
import EntityAccess;
import ScriptAccess;
import PhysicsEngine2D;

export namespace rke
{
    class ScriptManager
    {
    public:
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
    private:
        struct RuntimeCache
        {
            ScriptType script_type{ script_type_null };
            Scope<Script> script{};
        };

        Scope<Script> create_script(ScriptType type, EntityHandle owner);
        void destroy_script(Scope<Script> script);
        void refresh_cache(RuntimeCache& cache, ScriptType type, EntityHandle owner);

        enum class ContactType
        {
            SolidBegin,
            SolidEnd,
            SensorBegin,
            SensorEnd,
        };
        void contact_callback (
            EntityHandle owner_handle,
            EntityHandle other_handle,
            ContactType type
        );

        static void on_script_com_destroy(entt::registry& reg, entt::entity ent);
    private:
        Scene* owner_;
        std::vector<RuntimeCache> script_cache_{};
    };
}
