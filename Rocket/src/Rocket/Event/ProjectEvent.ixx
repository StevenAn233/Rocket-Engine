module;

#include "rke_macros.h"
#include "Events/event_macros.h"

export module ProjectEvent;

import Event;
import String;
import Types;

export namespace rke
{
    class RKE_API ProjectLoadedEvent : public Event
    {
    public:
        ProjectLoadedEvent(StringView name, void* project)
            : Event(name), project_(project) {}

        inline void* get_project() const { return project_; }

        EVENT_CLASS_TYPE(ProjectLoaded);
        EVENT_CLASS_CATEGORY(EventCategoryProject);
    private:
        void* project_;
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
        ProjectSavedEvent(StringView name, void* project)
            : Event(name), project_(project) {}

        inline void* get_project() const { return project_; }

        EVENT_CLASS_TYPE(ProjectSaved);
        EVENT_CLASS_CATEGORY(EventCategoryProject);
    private:
        void* project_;
    };

    class RKE_API ProjectSamplesSetEvent : public Event
    {
    public:
        ProjectSamplesSetEvent(StringView name, void* project, uint32 samples)
            : Event(name), project_(project), samples_(samples) {}
        
        inline void* get_project() const { return project_; }
        inline uint32 get_samples() const { return samples_; }

        EVENT_CLASS_TYPE(ProjectSamplesSet);
        EVENT_CLASS_CATEGORY(EventCategoryProject);
    private:
        void* project_;
        uint32 samples_;
    };
}
