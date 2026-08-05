module;

#include "rke_macros.h"
#include "Events/event_macros.h"

export module ProjectEvent;

import Event;
import String;

export namespace rke
{
    class RKE_API ProjectLoadedEvent : public Event
    {
    public:
        ProjectLoadedEvent(StringView name) : Event(name) {}

        EVENT_CLASS_TYPE(ProjectLoaded);
        EVENT_CLASS_CATEGORY(EventCategoryProject);
    };

    class RKE_API ProjectClearedEvent : public Event
    {
    public:
        ProjectClearedEvent(StringView name) : Event(name) {}

        EVENT_CLASS_TYPE(ProjectCleared);
        EVENT_CLASS_CATEGORY(EventCategoryProject);
    };

    class RKE_API ProjectSavedEvent : public Event
    {
    public:
        ProjectSavedEvent(StringView name) : Event(name) {}

        EVENT_CLASS_TYPE(ProjectSaved);
        EVENT_CLASS_CATEGORY(EventCategoryProject);
    };
}
