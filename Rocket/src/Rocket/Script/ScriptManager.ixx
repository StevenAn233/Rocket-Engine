module;

#include <vector>
namespace rke { class Scene; }

export module ScriptManager;

import Types;

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
        Scene* owner_;
        std::vector<void*> script_cache_{};
    };
}
