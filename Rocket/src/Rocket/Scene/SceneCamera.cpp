module;
module SceneCamera;

import Log;

namespace rke
{
    SceneCamera::SceneCamera() {}

    void SceneCamera::set_orthographic(float size, float near_clip, float far_clip)
    {
        orthographic_size_ = std::max(size, MIN_ORTHO_SIZE);
        orthographic_near_ = near_clip;
        orthographic_far_  = far_clip;
        update_proj();
    }

    void SceneCamera::set_perspective(float fov, float near_clip, float far_clip)
    {
        perspective_fov_  = glm::radians
            (glm::clamp(fov, MIN_VERTICAL_FOV, MAX_VERTICAL_FOV));
        perspective_near_ = near_clip;
        perspective_far_  = far_clip;
        update_proj();
    }

    void SceneCamera::set_viewport(uint32 w, uint32 h)
    {
        if(w == 0u || h == 0u) aspect_ratio_ = 0.0f;
        else aspect_ratio_ = static_cast<float>(w) / static_cast<float>(h);
        update_proj();
    }

    void SceneCamera::update_proj()
    {
        if(aspect_ratio_ == 0.0f || std::isnan(aspect_ratio_))
        {
            proj_ = glm::mat4(1.0f);
            ortho_bounds_ = {};
            return;
        }

        switch(type_)
        {
        case rke::SceneCamera::Type::Orthographic:
            ortho_bounds_.left   = -0.5f * orthographic_size_ * aspect_ratio_;
            ortho_bounds_.right  =  0.5f * orthographic_size_ * aspect_ratio_;
            ortho_bounds_.bottom = -0.5f * orthographic_size_;
            ortho_bounds_.top    =  0.5f * orthographic_size_;
            proj_ = glm::ortho(ortho_bounds_.left, ortho_bounds_.right,
                               ortho_bounds_.bottom, ortho_bounds_.top,
                               orthographic_near_, orthographic_far_ );
            break;
        case rke::SceneCamera::Type::Perspective:
            proj_ = glm::perspective(perspective_fov_ , aspect_ratio_,
                                     perspective_near_, perspective_far_);
            break;
        default:
            proj_ = glm::mat4(1.0f);
            break;
        }
    }
}
