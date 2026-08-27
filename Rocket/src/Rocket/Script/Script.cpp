module;
#include <glm/glm.hpp>

module Script;

import Log;
import Scene;

namespace rke
{
    Script::Script() : owner_() { /* CORE_WARN(u8"Script Constructed"); */ }
    Script::~Script() { /* CORE_WARN(u8"Script Destructed"); */ }

    Scene& Script::owner_scene()
    {
        CORE_ASSERT(owner_.get_owner(), u8"Script: Scene null!");
        return *(owner_.get_owner());
    };
}
