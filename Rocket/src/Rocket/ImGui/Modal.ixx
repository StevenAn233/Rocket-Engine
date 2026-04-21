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
        const String& get_title() const { return title_; }
    protected:
        Modal(String title) : title_(std::move(title)) {}
    private:
        String title_;
    };
}
