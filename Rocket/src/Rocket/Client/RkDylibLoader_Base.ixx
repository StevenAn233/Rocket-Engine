module;

#include <memory>
#include <utility>
#include <concepts>
#include <functional>
namespace rke { class ScriptRegistry; }

export module RkDylibLoader:Base;

import Path;
import String;
import Log;
import HeapManager;

export namespace rke
{
    using ScriptsRegistar = void(*)(ScriptRegistry*);

    class RkDylib
    {
    public:
        virtual ~RkDylib() = default;
        
        virtual bool is_valid() const = 0;
        virtual ScriptsRegistar get_scripts_registar() const = 0;
    protected:
        RkDylib() {};
    };

    class RkDylibLoader
    {
    public:
        RkDylibLoader(Path dir, String name);
        virtual ~RkDylibLoader() = default;

        // Returns whether the reload happened.
        template<typename Func>
        requires std::invocable<Func, Scope<RkDylib>, const RkDylib*>
        inline bool refresh(Func&& callback)
        {
            Scope<RkDylib> new_dylib{ load_new() };
            if(!new_dylib || !new_dylib->is_valid()
            || !new_dylib->get_scripts_registar()) return false;
            std::invoke(std::forward<Func>(callback),
                        std::move(current_dylib_), new_dylib.get());
            current_dylib_.reset(new_dylib.release());
            return true;
        }

        inline const RkDylib* get_current() const { return current_dylib_.get(); }
        static Scope<RkDylibLoader> create(Path dir, String name);
    protected:
        virtual Scope<RkDylib> load_new() = 0;
    protected:
        Path dylib_dir_{};
        String dylib_name_{};

        Scope<RkDylib> current_dylib_{};
        uint32 reload_count_{};
    };
}
