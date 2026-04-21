module;
module EditorCamera;

import Log;
import Input;
import Keys;
import MouseButtons;

namespace rke
{
    EditorCamera::EditorCamera() {}
    EditorCamera::EditorCamera(float fov, float aspect_ratio, float near_clip, float far_clip)
        : vertical_fov_(fov), aspect_ratio_(aspect_ratio)
        , near_clip_(near_clip), far_clip_(far_clip)
        , Camera(glm::perspective(glm::radians(fov), aspect_ratio, near_clip, far_clip))
    { update_view(); }

    EditorCamera::~EditorCamera() {}

    void EditorCamera::update_proj()
    {
        if(viewport_w_ == 0u || viewport_h_ == 0u)
        {
            aspect_ratio_ = 0.0f;
            proj_ = glm::mat4(1.0f);
            return;
        }
        aspect_ratio_ = static_cast<float>(viewport_w_) / viewport_h_;
        proj_ = glm::perspective(glm::radians(vertical_fov_),
                                 aspect_ratio_, near_clip_, far_clip_);
    }

    void EditorCamera::update_view()
    {
        // yaw_ = pitch_ = 0.0f; // Lock the camera's rotation
        position_ = calculate_pos();
        view_ = glm::inverse(glm::translate(glm::mat4(1.0f), position_) *
                             glm::mat4_cast(get_orientation()));
    }

    void EditorCamera::on_update(float dt)
    {
        glm::vec2 mouse {
            Input::get_mouse_pos_in_window().first,
            Input::get_mouse_pos_in_window().second
        };
        glm::vec2 delta{ (mouse - last_mouse_pos_) * 0.003f };
        last_mouse_pos_ = mouse;

        if(Input::is_mouse_button_pressed(Mouse::Middle))
        {
            if(Input::is_key_pressed(Key::LeftShift))
                mouse_pan(delta);
            else mouse_rotate(delta);
        }

        update_view();
    }

    bool EditorCamera::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        float delta{ e.get_y_offset() * 0.1f };
        mouse_zoom(delta);
        update_view();
        return false;
    }

    void EditorCamera::set_viewport(uint32 width, uint32 height)
    {
        viewport_w_ = width;
        viewport_h_ = height;
        update_proj();
    }

    void EditorCamera::mouse_pan(glm::vec2 delta)
    {
        auto [x_speed, y_speed] { pan_speed() };
        focal_point_ += -get_right_dir() * delta.x * x_speed * distance_;
        focal_point_ +=  get_up_dir	  () * delta.y * y_speed * distance_;
    }

    void EditorCamera::mouse_rotate(glm::vec2 delta)
    {
        float yaw_sign{ get_up_dir().y < 0.0f ? -1.0f : 1.0f };
        yaw_   += yaw_sign * delta.x * rotation_speed();
        pitch_ += delta.y * rotation_speed();
    }

    void EditorCamera::mouse_zoom(float delta) // dolly
    {
        distance_ -= delta * zoom_speed();
        if(distance_ < 1.0f)
        {
            focal_point_ += get_forward_dir();
            distance_ = 1.0f;
        }
    }

    std::pair<float, float> EditorCamera::pan_speed() const
    {
        float x{ std::min(viewport_w_ / 1000.0f, 2.4f) }; // max = 2.4f
        float x_factor{ 0.0366f * (x * x) - 0.1778f * x + 0.3021f };

        float y{ std::min(viewport_h_ / 1000.0f, 2.4f) }; // max = 2.4f
        float y_factor{ 0.0366f * (y * y) - 0.1778f * y + 0.3021f };

        return { x_factor, y_factor };
    }

    float EditorCamera::rotation_speed() const { return 0.8f; }

    float EditorCamera::zoom_speed() const
    {
        float dis_factor{ distance_ * 0.2f };
        dis_factor = std::max(dis_factor, 0.0f);
        float speed{ dis_factor * dis_factor };
        speed = std::min(speed, 100.0f); // max speed = 100
        return speed;
    }

    glm::vec3 EditorCamera::get_up_dir() const
        { return get_orientation() * glm::vec3(0.0f, 1.0f, 0.0f); }
    glm::vec3 EditorCamera::get_right_dir() const
        { return get_orientation() * glm::vec3(1.0f, 0.0f, 0.0f); }
    glm::vec3 EditorCamera::get_forward_dir() const
        { return get_orientation() * glm::vec3(0.0f, 0.0f, -1.0f); }
    glm::quat EditorCamera::get_orientation() const
        { return glm::quat(glm::vec3(-pitch_, -yaw_, 0.0f)); }
    glm::vec3 EditorCamera::calculate_pos() const
        { return focal_point_ - get_forward_dir() * distance_; }

    void EditorCamera::reset()
    {
        focal_point_ = {};
        distance_ = 10.0f;
        pitch_ = 0.0f; yaw_ = 0.0f;
        update_view();
    }

    void EditorCamera::serialize_to(ConfigWriter* writer) const
    {
        writer->begin_map(u8"Editor Camera");
        writer->write(u8"Focal Point", focal_point_);
        writer->write(u8"Distance", distance_);
        writer->write(u8"Pitch", pitch_);
        writer->write(u8"Yaw", yaw_);
        writer->end_map();
    }

    void EditorCamera::deserialize_from(const ConfigReader* reader)
    {
        auto cam_config{ reader->get_child(u8"Editor Camera") };
        if(!cam_config) { reset(); return; } // for just-created scenes
        if(!cam_config || !cam_config->is_map()) {
            CORE_ERROR(u8"EditorCamera: File format incorrect!");
            reset(); return;
        }
        focal_point_ = cam_config->get_at(u8"Focal Point", focal_point_);
        distance_ = cam_config->get_at(u8"Distance", distance_);
        pitch_ = cam_config->get_at(u8"Pitch", pitch_);
        yaw_ = cam_config->get_at(u8"Yaw", yaw_);
    }
}
