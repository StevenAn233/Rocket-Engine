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

        void refresh_script(uint32 handle);
        void dispatch_contacts(const std::vector<Contact>& begin_events,
                               const std::vector<Contact>& end_events);
    private:
        Scope<Script> create_script(uint32 handle);
        void destroy_script(Scope<Script> script, uint32 handle);

        void contact_begin(uint32 owner_handle, uint32 other_handle);
        void contact_end  (uint32 owner_handle, uint32 other_handle);

        static void on_script_com_destroy(entt::registry& reg, entt::entity ent);
    private:
        Scene* owner_;
        std::unordered_map<uint32, Scope<Script>> script_cache_{};
    };
}
