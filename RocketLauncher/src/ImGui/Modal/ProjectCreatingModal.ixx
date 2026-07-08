module;
export module ProjectCreatingModal;

import rke;
import Modal;

export namespace rke
{
    class ProjectCreatingModal : public Modal
    {
    public:
        using ProjectCreatedCallback = std::function<void(const Path&)>;

        ProjectCreatingModal(String title, const Window* context);

        void set_project_created_callback(ProjectCreatedCallback callback)
            { on_project_created_ = std::move(callback); }
    private:
        void on_imgui_render() override;
        void copy_to_buffer(const String& path);
    private:
        const Window* context_;
        ProjectCreatedCallback on_project_created_{};
        Scope<std::array<char, 256>> name_buffer_{};
        Scope<std::array<char, 512>> path_buffer_{};
    };
}
