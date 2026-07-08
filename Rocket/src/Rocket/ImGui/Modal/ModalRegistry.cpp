module;
module ModalRegistry;

import Log;

namespace rke
{
    void ModalRegistry::register_modal(Modal* handle, Attrib attrib)
    {
        if(attribs_.contains(handle))
        {
            CORE_ERROR(u8"ModalRegistry: Modal already registered!");
            return;
        }
        attribs_.emplace(handle, attrib);
    }

    void ModalRegistry::unregister_modal(Modal* handle)
    {
        if(attribs_.contains(handle))
            attribs_.erase(handle);
    }

    void ModalRegistry::render_all()
    {
        for(const auto& [handle, attrib] : attribs_)
        {
            if(attrib.popup_condition())
                handle->popup();
            handle->on_imgui_render();
        }
    }
}
