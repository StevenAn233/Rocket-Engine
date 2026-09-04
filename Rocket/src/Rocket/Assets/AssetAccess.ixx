module;

#include "rke_macros.h"

export module AssetAccess;

import UUID;
import Types;

export namespace rke
{
    using AssetUUID = UUID;
    
    enum class AssetHandle : uint64 {};
    constexpr AssetHandle asset_handle_null
        { static_cast<AssetHandle>(0xFFFFFFFFFFFFFFFFull) };

    struct RKE_API AssetResolve
    {
        AssetUUID uuid{ 0 };
        AssetHandle handle{ asset_handle_null };
    };
}
