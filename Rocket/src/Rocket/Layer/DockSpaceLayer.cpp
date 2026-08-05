module;
module DockSpaceLayer;

import EventDispatcher;
import KeyEvent;

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
        dispacher.dispatch<KeyPressedEvent> (
            [this](KeyPressedEvent& e)
                { return dockspace_.on_key_pressed(e); });
        
        if(e.belongs_to(EventCategoryMouse))
            dispacher.mark_completed_if(should_block_mouse());
        else if(e.belongs_to(EventCategoryKeyboard))
            dispacher.mark_completed_if(should_block_keyboard());
    }

    void DockSpaceLayer::on_update(float dt)
    {
        Layer::on_update(dt);
        dockspace_.on_update();
    }

    void DockSpaceLayer::on_render()
        { dockspace_.render(glm::vec2(0.0f), glm::vec2(1.0f)); }

    bool DockSpaceLayer::should_block_mouse()
        { return dockspace_.should_block_mouse(); }

    bool DockSpaceLayer::should_block_keyboard()
        { return dockspace_.should_block_keyboard(); }
}
