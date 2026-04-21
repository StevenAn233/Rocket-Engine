module;

#include "rke_macros.h"

export module DeltaTime;

import Types;

export namespace rke
{
    class DeltaTime // TO MODIFY
    {
    public:
        static void update();
        static void update_smoothed_fps();
        static void update_slow_fps();

        static RKE_API float  get();
        static RKE_API uint32 get_fps();
        static RKE_API uint32 get_slow_fps();
        static RKE_API uint32 get_smoothed_fps();
    };
}
