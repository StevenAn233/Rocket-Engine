module;

#include <concepts>
#include "rke_macros.h"

export module EventDispatcher;

import Event;

export namespace rke
{
    class RKE_API EventDispatcher
    {
    public:
        EventDispatcher(Event& event) : event_(event) {}

        template<typename E, typename Func>
        requires std::derived_from<E, Event>
              && std::same_as<std::invoke_result_t<Func, E&>, bool>
        bool dispatch(Func&& callback)
        {
            if(event_.get_type_id() == E::type_id())
            {
                event_.handled_ |= std::invoke
                    (std::forward<Func>(callback), static_cast<E&>(event_));
                return true;
            }
            return false;
        }

        void mark_completed_if(bool cond) { event_.handled_ |= cond; }
    private:
        Event& event_;
    };
}
