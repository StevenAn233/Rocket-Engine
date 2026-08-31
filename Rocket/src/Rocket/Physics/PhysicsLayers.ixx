module;

#include <optional>
#include <array>
#include "rke_macros.h"

export module PhysicsLayers;

import Types;
import String;

export namespace rke
{
    class PhysicsLayers
    {
    public:
        friend class Project;

        PhysicsLayers() = default;
        ~PhysicsLayers() = default;

        RKE_API void set_collision(uint8 layer_a, uint8 layer_b, bool should_collide);
        RKE_API void set_name(uint8 layer, String name);
        RKE_API void set_mask(uint8 layer, uint16 mask);
        RKE_API void set_showed_layer_count(uint8 layer);

        RKE_API uint16 get_category_bit(uint8 layer) const; // 0 to 15
        RKE_API std::optional<uint8> get_layer_index(const String& layer_name) const;
        RKE_API const String& get_name(uint8 layer) const;
        RKE_API uint16 get_mask(uint8 layer) const;

        RKE_API bool if_collides(uint8 layer_a, uint8 layer_b) const;

        inline uint8 get_showed_layer_count() const { return showed_layers_count_; }
        inline void plus_showed_layer_count()
            { if(showed_layers_count_ < (max_layers - 1u)) showed_layers_count_++; }
        inline void minus_showed_layer_count()
            { if(showed_layers_count_ > 1u) showed_layers_count_--; }
    private:
        using Layer = std::pair<String, uint16>;

        static constexpr uint8 max_layers{ 16ui8 };
        static constexpr const char8* default_name{ u8"Null" };
        static constexpr uint16 default_mask{ 0xFFFF };

        std::array<Layer, max_layers> layers_{ Layer(default_name, default_mask) };
        uint8 showed_layers_count_{ 1u };
    };
}
