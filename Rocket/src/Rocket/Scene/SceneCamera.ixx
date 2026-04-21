module;

#include <glm/glm.hpp>
#include "rke_macros.h"

export module SceneCamera;

import Types;
import Camera;

export namespace rke
{
    class RKE_API SceneCamera : public Camera
    {
    public:
        enum class Type { Perspective = 0, Orthographic };

        struct OrthoBounds // TO REMOVE
        {
            float left{}, right{};
            float top{}, bottom{};

            float get_width () const { return right - left; }
            float get_height() const { return top - bottom; }
        };

        SceneCamera();

    // getters
        float get_orthographic_size		() const { return orthographic_size_; }
        float get_orthographic_near_clip() const { return orthographic_near_; }
        float get_orthographic_far_clip () const { return orthographic_far_ ; }

        float get_perspective_vertical_fov() const { return glm::degrees(perspective_fov_); }
        float get_perspective_near_clip   () const { return perspective_near_; }
        float get_perspective_far_clip    () const { return perspective_far_ ; }

        Type  get_current_type	  () const { return type_; }
        int   get_current_type_int() const { return static_cast<int>(type_); }

        const OrthoBounds& get_bounds() const { return ortho_bounds_; }
    // setters
        void set_orthographic_size(float size)
            { orthographic_size_ = std::max(size, MIN_ORTHO_SIZE); update_proj(); }
        void set_orthographic_near_clip(float near) { orthographic_near_ = near; update_proj(); }
        void set_orthographic_far_clip (float far ) { orthographic_far_  = far ; update_proj(); }
        void set_orthographic(float size, float near_clip, float far_clip);

        void set_perspective_vertical_fov(float fov)
        {
            perspective_fov_ = glm::radians
                (glm::clamp(fov, MIN_VERTICAL_FOV, MAX_VERTICAL_FOV));
            update_proj();
        }
        void set_perspective_near_clip(float near) { perspective_near_ = near; update_proj(); }
        void set_perspective_far_clip (float far ) { perspective_far_  = far ; update_proj(); }
        void set_perspective(float fov, float near_clip, float far_clip);

        void set_viewport(uint32 w, uint32 h);
        void set_current_type(int type)
            { type_ = static_cast<Type>(type); update_proj(); }
    private:
        void update_proj();
    private:
        Type type_{ Type::Orthographic };
        float aspect_ratio_{};

        OrthoBounds ortho_bounds_{};
        float orthographic_size_{  10.0f };
        float orthographic_near_{ -10.0f }, orthographic_far_{ 10.0f };

        float perspective_fov_ { glm::radians(45.0f) };
        float perspective_near_{ 0.01f }, perspective_far_{ 100.0f };
    public:
        static constexpr float MIN_ORTHO_SIZE  { 0.01f  };
        static constexpr float MIN_VERTICAL_FOV{ 1.0f   };
        static constexpr float MAX_VERTICAL_FOV{ 160.0f };
    };
}
