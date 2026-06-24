module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module Camera;

export namespace rke
{
    class RKE_API Camera
    {
    public:
        const glm::mat4& get_proj() const { return proj_; }
    protected:
        Camera() : proj_({ 1.0f }) {}
        Camera(glm::mat4 mat) : proj_(std::move(mat)) {}
        virtual ~Camera() = default;
    protected:
        glm::mat4 proj_;
    };
}
