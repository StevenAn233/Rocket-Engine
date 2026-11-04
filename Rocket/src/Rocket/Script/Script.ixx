module;

#include <bit>
#include <type_traits>
#include "rke_macros.h"
namespace rke { class Entity; class Scene; }

export module Script;

import Types;
import EntityAccess;
import ConfigProxy;
import ScriptAccess;

export namespace rke
{
    class Script
    {
    public:
        friend class ScriptManager;
        friend class ScriptRegistry;

        RKE_API Script();
        RKE_API Script(const Script& other);
        RKE_API virtual ~Script();

        virtual void on_create () {}
        virtual void on_destroy() {}

        virtual void on_runtime_start() {}
        virtual void on_runtime_stop () {}

        virtual void on_serialize(ConfigWriter& writer) const {}
        virtual void on_deserialize(const ConfigReader& reader) {}

        virtual void on_update(double dt) {}
        virtual void on_imgui_render() {}

        virtual void on_mouse_scrolled(float x_offset, float y_offset) {}
        virtual void on_contact_solid_begin(EntityHandle other) {}
        virtual void on_contact_solid_end  (EntityHandle other) {}
        virtual void on_contact_sensor_begin(EntityHandle other) {}
        virtual void on_contact_sensor_end  (EntityHandle other) {}

        virtual const char8_t* get_object_type_name() const = 0;
        inline ScriptType get_script_type() const
            { return std::bit_cast<ScriptType>(get_object_type_name()); }
    protected:
        inline Entity& owner() { return *(std::bit_cast<Entity*>(uintptr(&padding_))); }
        inline EntityHandle owner_handle() const { return owner_handle_; }
        inline Scene* owner_scene() { return owner_scene_; }
    private: // set by ScriptManager
        uint32 padding_{};
        EntityHandle owner_handle_{ entity_handle_null };
        Scene* owner_scene_{};
    };

    template<typename T>
    inline ScriptType script_type_of()
    {
        const char8_t* name{ T::get_static_type_name() };
        return std::bit_cast<ScriptType>(name);
    }

    template<typename T>
    requires std::is_base_of_v<Script, T>
    inline T* script_cast(Script* script)
    {
        if(!script) return nullptr;
        const ScriptType expected{ script_type_of<T>() };
        if(expected == script_type_null) return nullptr;
        if(script->get_script_type() != expected) return nullptr;
        return static_cast<T*>(script); // the type matched, so this is exact
    }

    template<typename T>
    requires std::is_base_of_v<Script, T>
    inline const T* script_cast(const Script* script)
    {
        if(!script) return nullptr;
        const ScriptType expected{ script_type_of<T>() };
        if(expected == script_type_null) return nullptr;
        if(script->get_script_type() != expected) return nullptr;
        return static_cast<const T*>(script);
    }
}
