module;

#include <utility>
#include "rke_macros.h"

export module Modal;

import String;

export namespace rke
{
    class RKE_API Modal
    {
    public:
        Modal(String title) : title_(std::move(title)) {}
        virtual ~Modal() = default;

        virtual void on_imgui_render() = 0;

        void popup();
        inline const String& get_title() const { return title_; }
        inline bool in_use() const { return in_use_; }
    protected:
        String title_;
        bool in_use_{ false };
    };
}
