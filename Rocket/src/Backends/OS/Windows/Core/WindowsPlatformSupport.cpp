module;

#include <windows.h>

module PlatformSupport;

import Log;

namespace rke::platform_support
{
    void begin()
    {
        HRESULT hr{ CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED) };
        if(FAILED(hr)) CORE_ERROR(u8"WindowsPlatformSupport: Failed to co-initialize!");
    }

    void end() { CoUninitialize(); }
}
