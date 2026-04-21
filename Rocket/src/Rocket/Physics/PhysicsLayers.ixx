module;

#include <optional>
#include <array>
#include "rke_macros.h"

export module PhysicsLayers;

import Types;
import String;

export namespace rke
{
    class RKE_API PhysicsLayers
    {
    public:
        struct Layer
        {
            String name{ u8"Null" };
            uint16 mask{  0xFFFF  };
        };

        static constexpr uint8  get_layer_count () { return 16ui8;    }
        static constexpr String get_default_name() { return u8"Null"; }
        static constexpr uint16 get_default_mask() { return 0xFFFF;   }

        uint16 get_category_bit(uint8 layer) const; // 0 to 15

        void set_collision(uint8 layer_a, uint8 layer_b, bool should_collide);
        void set_name(uint8 layer, String name);
        void set_mask(uint8 layer, uint16 mask);

        std::optional<uint8> get_layer_index(const String& layer_name) const;
        const String& get_name(uint8 layer) const;
        uint16 get_mask(uint8 layer) const;

        void  set_showed_layer_count(uint8 layer);
        uint8 get_showed_layer_count() const { return showed_layer_count_; }
        void plus_showed_layer_count()
        {
            if(showed_layer_count_ < (get_layer_count() - 1u))
                showed_layer_count_++;
        }
        void minus_showed_layer_count()
        {
            if(showed_layer_count_ > 1u)
                showed_layer_count_--;
        }

        bool if_collides(uint8 layer_a, uint8 layer_b);
    private:
        std::array<Layer, 16> layers_{};
        uint8 showed_layer_count_{ 1u };
    };
}
