module;

#include <concepts>
#include "rke_macros.h"

export module EventDispatcher;

import Event;

export namespace rke
{
    class RKE_API EventDispatcher // Accessible to handled_(protected)
    {
    public:
        EventDispatcher(Event& event) : event_(event) {}

        template<typename E, typename Func>
        requires std::derived_from<E, Event>
              && std::same_as<std::invoke_result_t<Func, E&>, bool>
        bool dispatch(Func&& callback)
        {
            if(event_.get_event_type() == E::get_static_type())
            {
                event_.handled_ |= std::invoke
                    (std::forward<Func>(callback), static_cast<E&>(event_));
                return true;
            }
            return false;
        }
    private:
        Event& event_;
    };
}
