#ifndef RKE_SCRIPTING_H
#define RKE_SCRIPTING_H

#include <cstdint>
namespace rke { class ScriptEntry; }

extern "C"
{
    typedef struct {
        const rke::ScriptEntry* data;
        uint64_t size;
    } ScriptsView;
}

#endif // RKE_SCRIPTING_H
