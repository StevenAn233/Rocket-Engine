module;
module Script;

import Log;

namespace rke
{
    Script::Script() { CORE_TRACE(u8"Script constructed."); }
    Script::Script(const Script& other) : script_type_(other.script_type_)
        { CORE_TRACE(u8"Script copied."); }
    Script::~Script() { CORE_TRACE(u8"Script destroyed."); }
}
