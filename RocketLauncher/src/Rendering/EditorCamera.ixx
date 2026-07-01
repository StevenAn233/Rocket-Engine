module;
export module EditorCamera;

import rke;

export namespace rke
{
    class EditorCamera : public Camera
    {
    public:
        EditorCamera();
        EditorCamera(float fov, float aspect_ratio, float near_clip, float far_clip);
        ~EditorCamera();

        void on_update(float dt);
        bool on_mouse_scrolled(MouseScrolledEvent& e);

        float get_distance() const { return distance_; }
        void  set_distance(float distance) { distance_ = distance; }

        void set_viewport(uint32 width, uint32 height);

        const glm::mat4& get_view() const { return view_; }
        glm::mat4 get_view_proj  () const { return proj_ * view_; }

        glm::vec3 get_up_dir	 () const;
        glm::vec3 get_right_dir	 () const;
        glm::vec3 get_forward_dir() const;
        glm::quat get_orientation() const;
        glm::vec3 get_pos() const { return position_; }

        float get_pitch() const { return pitch_; }
        float get_yaw  () const { return yaw_  ; }

        void reset();

        void serialize_to(ConfigWriter* writer) const;
        void deserialize_from(const ConfigReader* reader);
    private:
        void update_proj();
        void update_view();

        void mouse_pan	 (glm::vec2 delta);
        void mouse_rotate(glm::vec2 delta);
        void mouse_zoom	 (float delta);

        glm::vec3 calculate_pos() const;

        std::pair<float, float> pan_speed() const;
        float rotation_speed() const;
        float zoom_speed	() const;
    private:
        float vertical_fov_{ 45.0f }, aspect_ratio_{};
        float near_clip_   { 0.01f }, far_clip_{ 1000.0f };

        glm::mat4 view_{ 1.0f };

        glm::vec3 position_	  { 0.0f, 0.0f, 0.0f };
        glm::vec3 focal_point_{ 0.0f, 0.0f, 0.0f };
        // rotation & zooming central, only moves when panning

        glm::vec2 last_mouse_pos_{ 0.0f, 0.0f };

        float distance_{ 10.0f }; // between focal point and camera
        float pitch_{}, yaw_{};

        uint32 viewport_w_{}, viewport_h_{};
    };
}
