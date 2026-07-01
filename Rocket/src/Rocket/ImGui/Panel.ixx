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
        inline void render() { if(on()) on_imgui_render(); }

        inline const String& get_name() const { return name_; }
        inline bool on() const { return on_; }

        inline void turn_on () { on_ = true;  }
        inline void turn_off() { on_ = false; }
    protected:
        Panel(String name) : name_(std::move(name)) {}
    private:
        String name_;
        bool on_{ true };
    };
}
