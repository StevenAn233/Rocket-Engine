module;
module Script;

import Log;

namespace rke
{
    Script::Script() { CORE_TRACE(u8"Script constructed."); }
    Script::Script(const Script& other) { CORE_TRACE(u8"Script copied."); }
    Script::~Script() { CORE_TRACE(u8"Script destroyed."); }
}
