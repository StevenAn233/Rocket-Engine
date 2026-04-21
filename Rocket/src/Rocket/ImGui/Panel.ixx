module;

#include <utility>
#include "rke_macros.h"

export module Panel;

import String;

export namespace rke
{
    class RKE_API Panel
    {
    public:
        friend class PanelRegistry;

        virtual ~Panel() = default;

        virtual void on_imgui_render() = 0;

        const String& get_name() const { return name_; }
        bool enabled() const { return opened_; }
    protected:
        Panel(String name) : name_(std::move(name)){}
    private:
        String name_;
        bool opened_{ true };
    };
}
