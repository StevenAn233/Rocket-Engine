module;

#include <vector>
#include <utility>
#include "rke_macros.h"

export module LogPanel;

import Panel;
import String;

export namespace rke
{
    class LogPanel : public Panel
    {
    public:
        LogPanel(String name) : Panel(std::move(name)) {}

        void push_text(String text);
    private:
        RKE_API void on_imgui_render() override;
    private:
        
    };
}