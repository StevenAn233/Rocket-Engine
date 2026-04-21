module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module Gravity2D;

export namespace rke
{
    class RKE_API Gravity2D
    {
    public:
        static glm::vec2 get_default() { return { 0.00f, -9.81f }; }

        glm::vec2 get() const { return gravity_; }
        glm::vec2& get_mut()  { return gravity_; }

        void set_to(float x, float y) { gravity_.x = x; gravity_.y = y; }
    private:
        glm::vec2 gravity_{ 0.00f, -9.81f };
    };
}
