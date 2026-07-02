module;

#include <windows.h>

module PlatformSupport;

import Log;

namespace rke
{
    void PlatformSupport::init()
    {
        HRESULT hr{ CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED) };
        if(FAILED(hr)) CORE_ERROR(u8"WindowsPlatformSupport: Failed to co-initialize!");
    }

    void PlatformSupport::shutdown() { CoUninitialize(); }
}
