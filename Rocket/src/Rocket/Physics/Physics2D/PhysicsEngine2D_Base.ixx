module;

#include <entt/entt.hpp>
namespace rke { class Scene; }

export module PhysicsEngine2D:Base;

import HeapManager;

export namespace rke
{
    class PhysicsEngine2D
    {
    public:
        PhysicsEngine2D(Scene* scene);
        virtual ~PhysicsEngine2D() = default;

        virtual void on_runtime_start() = 0;
        virtual void on_runtime_stop () = 0;

        virtual void on_update(float dt) = 0;

        virtual bool empty() const = 0;
        static Scope<PhysicsEngine2D> create(Scene* owner);
    protected:
        inline Scene& get_owner() { return *owner_; }
        entt::registry& get_registry();
    private:
        Scene* owner_;
    };
}
