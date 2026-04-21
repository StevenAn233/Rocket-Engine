module;

#include "rke_macros.h"

export module Script;

import Scene;
import Event;

export namespace rke
{
    class RKE_API Script
    {
    public:
        friend class Scene;
        friend class ScriptEngine;

        virtual ~Script() = default;

        template<typename Component>
        const bool has() { return context_.has<Component>(); }
        template<typename Component>
        const Component& get() { return context_.get<Component>(); }
        template<typename Component>
        Component& get_mut() { return context_.get_mut<Component>(); }
    protected:
        virtual void on_create () {}
        virtual void on_destroy() {}

        virtual void on_update(float dt) {}

        virtual void on_mouse_scrolled(MouseScrolledEvent& e) {};
    private:
        Entity context_{}; // initialized after constructed in Scene
    };
}
