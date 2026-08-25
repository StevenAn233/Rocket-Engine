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
    
        virtual void on_update(double dt) {}
        virtual void on_mouse_scrolled(float x_offset, float y_offset) {};
    protected:
        inline Entity owner() const { return owner_; }
    private:
        Entity owner_; // init by ScriptManager
    };
}
