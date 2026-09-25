module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module Gravity;

export namespace rke
{
    class RKE_API Gravity
    {
    public:
        static glm::vec3 get_default() { return { 0.00f, -9.81f, 0.00f }; }

        glm::vec3 get() const { return gravity_; }
        glm::vec3& get_mut() { return gravity_; }

        inline void set_to(float x, float y, float z) { gravity_ = { x, y, z }; }
    private:
        glm::vec3 gravity_{ 0.00f, -9.81f, 0.00f };
    };
}
