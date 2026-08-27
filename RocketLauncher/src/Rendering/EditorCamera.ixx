module;
export module EditorCamera;

import rke;

export namespace rke
{
    class EditorCamera : public Camera
    {
    public:
        friend class EditorSettingPanel;

        EditorCamera(float fov = 45.0f, float aspect_ratio = 0.0f,
                     float near_clip = 0.01f, float far_clip = 1000.0f);
        ~EditorCamera();

        void on_update(double dt);
        bool on_mouse_scrolled(MouseScrolledEvent& e);

        float get_distance() const { return distance_; }
        void  set_distance(float distance) { distance_ = distance; }

        void set_viewport_size(glm::vec2 size);

        const glm::mat4& get_view() const { return view_; }
        glm::mat4 get_view_proj() const { return proj_ * view_; }

        glm::vec3 get_up_dir() const;
        glm::vec3 get_right_dir() const;
        glm::vec3 get_forward_dir() const;
        glm::quat get_orientation() const;
        glm::vec3 calculate_pos() const;
        
        float get_pitch() const { return pitch_; }
        float get_yaw() const { return yaw_; }

        void reset();

        void serialize_to(ConfigWriter& writer) const;
        void deserialize_from(const ConfigReader& reader);
    private:
        void update_view();

        void mouse_pan	 (glm::vec2 delta);
        void mouse_rotate(glm::vec2 delta);
        void mouse_zoom	 (float delta);

        std::pair<float, float> pan_speed() const;
        float rotation_speed() const;
        float zoom_speed() const;
    private:
    // for projection matrix
        float vertical_fov_;
        float aspect_ratio_; // based on viewport size
        float near_clip_, far_clip_;
        glm::vec2 viewport_size_{ 0.0f }; // assumed positive

        glm::mat4 view_{ 1.0f };
    // for view matrix
        // rotation & zooming central, only moves when panning
        glm::vec3 focus_{ 0.0f, 0.0f, 0.0f };
        float distance_{ 10.0f }; // between focal point and camera
        float pitch_{}, yaw_{}; // radian

        glm::vec2 last_mouse_pos_{ 0.0f, 0.0f };
    };
}
