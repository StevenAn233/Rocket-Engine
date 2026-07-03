module;
module DockSpaceLayer;

namespace rke
{
    DockSpaceLayer::DockSpaceLayer(String name, void* owner, Path config_path)
        : Layer(std::move(name), owner)
        , dockspace_(u8"Rocket Dockspace", std::move(config_path)) {}

    void DockSpaceLayer::on_event(Event& e)
    {
        if(e.belongs_to(EventCategoryMouse))
            e.handled_ |= should_block_mouse();
        if(e.belongs_to(EventCategoryKeyboard))
            e.handled_ |= should_block_keyboard();
    }

    void DockSpaceLayer::on_render()
        { dockspace_.render(glm::vec2(0.0f), glm::vec2(1.0f)); }

    bool DockSpaceLayer::should_block_mouse()
        { return dockspace_.mouse_blocking(); }

    bool DockSpaceLayer::should_block_keyboard()
        { return dockspace_.keyboard_blocking(); }
}
