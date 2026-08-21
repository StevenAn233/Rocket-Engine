module;

#include "rke_macros.h"

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
    
        virtual void on_update(float dt) {}
        virtual void on_mouse_scrolled(float x_offset, float y_offset) {};

        template<typename Component>
        inline const bool has() { return owner_.has<Component>(); }
        template<typename Component>
        const Component& get() { return owner_.get<Component>(); }
        template<typename Component>
        Component& get_mut() { return owner_.get_mut<Component>(); }
    protected:
        Entity owner_; // init by ScriptManager
    };
}
