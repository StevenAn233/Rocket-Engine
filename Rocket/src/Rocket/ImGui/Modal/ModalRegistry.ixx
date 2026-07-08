module;

#include <functional>
#include <unordered_map>

export module ModalRegistry;

import Modal;

export namespace rke
{
    class ModalRegistry
    {
    public:
        friend class DockSpace;

        struct Attrib { std::function<bool()> popup_condition; };

        ModalRegistry() = default;
        ~ModalRegistry() = default;

        ModalRegistry(const ModalRegistry&) = delete;
        ModalRegistry& operator=(const ModalRegistry&) = delete;
        ModalRegistry(ModalRegistry&&) = delete;
        ModalRegistry& operator=(ModalRegistry&&) = delete;

        void register_modal(Modal* handle, Attrib attrib);
        void unregister_modal(Modal* handle);
    private:
        void render_all();
    private:
        std::unordered_map<Modal*, Attrib> attribs_;
    };
}
