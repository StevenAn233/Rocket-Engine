module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module Gravity;

export namespace rke
{
    class RKE_API Gravity
    {
    public:
        Gravity(glm::vec3 val = default_val()) : data_(val) {}

        Gravity(const Gravity&) = default;
        Gravity& operator=(const Gravity&) = default;
        Gravity(Gravity&&) = default;
        Gravity& operator=(Gravity&&) = default;

        glm::vec3 val() const { return data_; }
        glm::vec3& ref() { return data_; }

        inline void set_to(float x, float y, float z) { data_ = { x, y, z }; }
        static glm::vec3 default_val() { return { 0.00f, -9.81f, 0.00f }; }
    private:
        glm::vec3 data_;
    };
}
