module;
module EditorCamera;

import Log;
import Input;
import Keys;
import MouseButtons;

namespace rke
{
    EditorCamera::EditorCamera(float fov, float aspect_ratio, float near_clip, float far_clip)
        : Camera(glm::perspective(glm::radians(fov), aspect_ratio, near_clip, far_clip))
        , vertical_fov_(fov), aspect_ratio_(aspect_ratio)
        , near_clip_(near_clip), far_clip_(far_clip) { update_view(); }

    EditorCamera::~EditorCamera() {}

    void EditorCamera::update_view()
    {
        // view_props_.yaw = view_props_.pitch = 0.0f; // Lock the camera's rotation
        constexpr float twopi{ 2.0f * float(std::numbers::pi) };
        while(view_props_.pitch < 0) view_props_.pitch += twopi;
        while(view_props_.yaw   < 0) view_props_.yaw   += twopi;
        while(view_props_.pitch > twopi) view_props_.pitch -= twopi;
        while(view_props_.yaw   > twopi) view_props_.yaw   -= twopi;
        view_ = glm::inverse (
            glm::translate(glm::mat4(1.0f), calculate_pos()) *
            glm::mat4_cast(get_orientation())
        );
    }

    void EditorCamera::on_update(double dt)
    {
        glm::vec2 mouse{ app().input().get_mouse_pos_in_window() };
        glm::vec2 delta{ (mouse - last_mouse_pos_) * 0.003f };
        last_mouse_pos_ = mouse;

        if(app().input().is_mouse_button_pressed(Mouse::Middle))
        {
            if(app().input().is_key_pressed(Key::LeftShift))
                mouse_pan(delta);
            else mouse_rotate(delta);
            update_view();
        }
    }

    void EditorCamera::on_imgui_render()
    {
        bool modified{ false };
        modified |= layout::drag_float3_control<u8"Focus">
            (view_props_.focus, 0.1f, glm::vec3(0.0f));
        modified |= layout::drag_float_control<u8"Distance">
            (view_props_.distance, 0.5f, 1.0f, glm::vec2(1.0f, 100.0f));
        modified |= layout::drag_float_control<u8"Pitch">(view_props_.pitch, 0.1f, 0.0f);
        modified |= layout::drag_float_control<u8"Yaw"  >(view_props_.yaw  , 0.1f, 0.0f);
        if(modified) update_view();
    }

    bool EditorCamera::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        float delta{ e.get_y_offset() * 0.1f };
        mouse_zoom(delta);
        update_view();
        return false;
    }

    void EditorCamera::set_viewport_size(glm::vec2 size)
    {
        if(size.x < 0.01f || size.y < 0.01f)
        {
            viewport_size_ = glm::vec2(0.0f);
            aspect_ratio_  = 0.0f;
            proj_ = glm::mat4(1.0f);
            return;
        }
        viewport_size_ = size;
        aspect_ratio_ = size.x / size.y;
        proj_ = glm::perspective (
            glm::radians(vertical_fov_),
            aspect_ratio_, near_clip_, far_clip_
        );
    }

    void EditorCamera::mouse_pan(glm::vec2 delta)
    {
        auto [x_speed, y_speed] { pan_speed() };
        view_props_.focus += -get_right_dir() * delta.x * x_speed * view_props_.distance;
        view_props_.focus +=  get_up_dir   () * delta.y * y_speed * view_props_.distance;
    }

    void EditorCamera::mouse_rotate(glm::vec2 delta)
    {
        float yaw_sign{ get_up_dir().y < 0.0f ? -1.0f : 1.0f };
        view_props_.yaw   += yaw_sign * delta.x * rotation_speed();
        view_props_.pitch += delta.y * rotation_speed();
    }

    void EditorCamera::mouse_zoom(float delta) // dolly
    {
        view_props_.distance -= delta * zoom_speed();
        if(view_props_.distance < 1.0f)
        {
        //  view_props_.focus += get_forward_dir();
            view_props_.distance = 1.0f;
        }
    }

    std::pair<float, float> EditorCamera::pan_speed() const
    {
        float x{ std::min(viewport_size_.x / 1000.0f, 2.4f) }; // max = 2.4f
        float x_factor{ 0.0366f * (x * x) - 0.1778f * x + 0.3021f };

        float y{ std::min(viewport_size_.y / 1000.0f, 2.4f) }; // max = 2.4f
        float y_factor{ 0.0366f * (y * y) - 0.1778f * y + 0.3021f };

        return { x_factor, y_factor };
    }

    float EditorCamera::rotation_speed() const { return 0.8f; }

    float EditorCamera::zoom_speed() const
    {
        float dis_factor{ view_props_.distance * 0.2f };
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
        { return glm::quat(glm::vec3(-view_props_.pitch, -view_props_.yaw, 0.0f)); }
    glm::vec3 EditorCamera::calculate_pos() const
        { return view_props_.focus - get_forward_dir() * view_props_.distance; }

    void EditorCamera::serialize_to(ConfigWriter& writer) const
    {
        writer.begin_map(u8"Editor Camera");
        writer.write(u8"Focus", view_props_.focus);
        writer.write(u8"Distance", view_props_.distance);
        writer.write(u8"Pitch", view_props_.pitch);
        writer.write(u8"Yaw", view_props_.yaw);
        writer.end_map();
    }

    void EditorCamera::deserialize_from(const ConfigReader& reader)
    {
        auto cam_config{ reader.get_child(u8"Editor Camera") };
        if(!cam_config) { reset(); return; } // for just-created scenes
        if(!cam_config || !cam_config->is_map()) {
            CORE_ERROR(u8"EditorCamera: File format incorrect!");
            reset(); return;
        }
        view_props_.focus = cam_config->get_at(u8"Focus", view_props_.focus);
        view_props_.distance = cam_config->get_at(u8"Distance", view_props_.distance);
        view_props_.pitch = cam_config->get_at(u8"Pitch", view_props_.pitch);
        view_props_.yaw = cam_config->get_at(u8"Yaw", view_props_.yaw);
        update_view();
    }
}
