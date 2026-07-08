module;
module DockSpaceLayer;

import EventDispatcher;

namespace rke
{
    DockSpaceLayer::DockSpaceLayer(String name, Window* owner, Path config_path)
        : Layer(std::move(name), owner)
        , dockspace_(u8"Rocket Dockspace",
            std::move(config_path), owner->get_context()) {}

    void DockSpaceLayer::on_event(Event& e)
    {
        if(e.handled()) return;
        EventDispatcher dispacher{ e };
        if(e.belongs_to(EventCategoryMouse))
            dispacher.mark_completed_if(should_block_mouse());
        else if(e.belongs_to(EventCategoryKeyboard))
            dispacher.mark_completed_if(should_block_keyboard());
    }

    void DockSpaceLayer::on_render()
        { dockspace_.render(glm::vec2(0.0f), glm::vec2(1.0f)); }

    bool DockSpaceLayer::should_block_mouse()
        { return dockspace_.mouse_blocking(); }

    bool DockSpaceLayer::should_block_keyboard()
        { return dockspace_.keyboard_blocking(); }
}
