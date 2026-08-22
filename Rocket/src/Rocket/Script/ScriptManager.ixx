module;

#include <memory>
#include <unordered_map>
namespace rke { class Scene; }

export module ScriptManager;

import Script;
import Types;
import HeapManager;

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
    private:
        Scope<Script> create_script(uint32 handle);
        void destroy_script(Scope<Script> script, uint32 handle);
    private:
        Scene* owner_;
        std::unordered_map<uint32, Scope<Script>> script_cache_{};
    };
}
