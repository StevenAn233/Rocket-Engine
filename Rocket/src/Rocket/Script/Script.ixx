module;

#include "rke_macros.h"
#include <glm/glm.hpp>

export module Script;

import Scene;
import Types;

export namespace rke
{
    class RKE_API Script
    {
    public:
        friend class ScriptManager;

        Script();
        virtual ~Script();

        virtual void on_create () {}
        virtual void on_destroy() {}
    
        virtual void on_update(double dt) {}

        virtual void on_mouse_scrolled(float x_offset, float y_offset) {};
        virtual void on_contact_solid_begin(Entity other) {}
        virtual void on_contact_solid_end  (Entity other) {}
        virtual void on_contact_sensor_begin(Entity other) {}
        virtual void on_contact_sensor_end  (Entity other) {}
    protected:
        inline Entity owner() const { return owner_; }
        Scene& owner_scene();
    private:
        Entity owner_; // init by ScriptManager
    };
}
