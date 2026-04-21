module;

#ifdef RKE_DEBUG
    #define CHECK_LAYER(layer) CORE_ASSERT ( \
        layer >= 0 && layer < get_layer_count(), \
        u8"PhysicsLayer: Layer must be within 0 to {}!", get_layer_count())
#else
    #define CHECK_LAYER(layer)
#endif

module PhysicsLayers;

import Log;

namespace rke
{
    uint16 PhysicsLayers::get_category_bit(uint8 layer) const
    {
        CHECK_LAYER(layer);
        return 1 << layer;
    }

    std::optional<uint8> PhysicsLayers::get_layer_index(const String& layer_name) const
    {
        for(uint8 i{}; i < get_layer_count(); i++)
            if(layers_[i].name == layer_name) return i;
        CORE_ERROR(u8"PhysicsLayers: Layer name '{}' not defined(initialized)!", layer_name);
        return std::nullopt;
    }

    void PhysicsLayers::set_collision(uint8 layer_a, uint8 layer_b, bool should_collide)
    {
        CHECK_LAYER(layer_a);
        CHECK_LAYER(layer_b);
        if(should_collide) {
            layers_[layer_a].mask |= get_category_bit(layer_b);
            layers_[layer_b].mask |= get_category_bit(layer_a);
            // setting: layer_a collides with layer_b; layer_b collides with layer_a;
        } else {
            layers_[layer_a].mask &= ~get_category_bit(layer_b);
            layers_[layer_b].mask &= ~get_category_bit(layer_a);
        }
    }

    void PhysicsLayers::set_name(uint8 layer, String name)
    {
        CHECK_LAYER(layer);
        layers_[layer].name = std::move(name);
    }

    void PhysicsLayers::set_mask(uint8 layer, uint16 mask)
    {
        CHECK_LAYER(layer);
        layers_[layer].mask = mask;
    }

    const String& PhysicsLayers::get_name(uint8 layer) const
    {
        CHECK_LAYER(layer);
        return layers_[layer].name;
    }

    uint16 PhysicsLayers::get_mask(uint8 layer) const
    {
        CHECK_LAYER(layer);
        return layers_[layer].mask;
    }

    void PhysicsLayers::set_showed_layer_count(uint8 layer)
    {
        CHECK_LAYER(layer);
        showed_layer_count_ = layer;
    }

    bool PhysicsLayers::if_collides(uint8 layer_a, uint8 layer_b)
    {
        CHECK_LAYER(layer_a);
        CHECK_LAYER(layer_b);
        // double-side when setting, so only need to check one side here
        return !!(get_mask(layer_a) & get_category_bit(layer_b));
    }                              // ^ this actually serves as mask here
}
