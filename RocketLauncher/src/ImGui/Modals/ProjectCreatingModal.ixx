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

        ProjectCreatingModal(String title);

        void popup();
        void on_render(const Window* window);
        void set_project_created_callback(ProjectCreatedCallback callback);

        bool in_use() const { return in_use_; }
    private:
        bool in_use_{ false };
        ProjectCreatedCallback on_project_created_{};
    };
}
