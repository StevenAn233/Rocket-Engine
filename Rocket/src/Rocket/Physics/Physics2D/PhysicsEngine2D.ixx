export module PhysicsEngine2D;

import Scene;

export namespace rke
{
    class PhysicsEngine2D
    {
    public:
        static void on_runtime_start(Scene* scene);
        static void on_runtime_stop ();
        static void on_update(float dt);
    };
}
