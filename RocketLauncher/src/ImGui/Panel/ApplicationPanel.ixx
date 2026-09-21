module;
export module ApplicationPanel;

import rke;

export namespace rke
{
    class ApplicationPanel : public Panel
    {
    public:
        ApplicationPanel(String name) : Panel(std::move(name)) {}
    private:
        void on_imgui_render() override;
    };
}
