module;

#include <bit>
#include "rke_macros.h"
namespace rke { class Entity; class Scene; }

export module Script;

import Types;
import EntityAccess;

export namespace rke
{
    class Script
    {
    public:
        friend class ScriptManager;

        RKE_API Script();
        RKE_API virtual ~Script();

        virtual void on_create () {}
        virtual void on_destroy() {}
    
        virtual void on_update(double dt) {}
        virtual void on_imgui_render() {}

        virtual void on_mouse_scrolled(float x_offset, float y_offset) {}
        virtual void on_contact_solid_begin(EntityHandle other) {}
        virtual void on_contact_solid_end  (EntityHandle other) {}
        virtual void on_contact_sensor_begin(EntityHandle other) {}
        virtual void on_contact_sensor_end  (EntityHandle other) {}
    protected:
        inline Entity& owner() { return *(std::bit_cast<Entity*>(uintptr_t(&padding_))); }
        inline EntityHandle owner_handle() const { return owner_handle_; }
        inline Scene* owner_scene() { return owner_scene_; }
    private: // set by ScriptManager
        uint32 padding_{};
        EntityHandle owner_handle_{ entity_handle_null };
        Scene* owner_scene_{};
    };
}
