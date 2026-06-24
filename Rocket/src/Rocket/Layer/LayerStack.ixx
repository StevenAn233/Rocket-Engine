module;

#include <memory>
#include <vector>
#include "rke_macros.h"

export module LayerStack;

import Types;
import HeapManager;
import Layer;

export namespace rke
{
    class RKE_API LayerStack
    {
    public:
        LayerStack() {};
        LayerStack(const LayerStack&) = delete;
        LayerStack& operator=(const LayerStack&) = delete;
        ~LayerStack();

        void push_layer(Scope<Layer> layer);      // put layer to the first-half
        Scope<Layer> pop_layer(Layer* layer);     // pop specified first-half-layer
        void push_overlay(Scope<Layer> overlay);  // put layer to the second-half
        Scope<Layer> pop_overlay(Layer* overlay); // pop specified second-half-layer

        inline std::vector<Scope<Layer>>::iterator begin() { return layers_.begin(); }
        inline std::vector<Scope<Layer>>::iterator end() { return layers_.end(); }
        inline std::vector<Scope<Layer>>::reverse_iterator rbegin() { return layers_.rbegin(); }
        inline std::vector<Scope<Layer>>::reverse_iterator rend() { return layers_.rend(); }
        
        inline std::vector<Scope<Layer>>::const_iterator cbegin() const { return layers_.cbegin(); }
        inline std::vector<Scope<Layer>>::const_iterator cend() const { return layers_.cend(); }
        inline std::vector<Scope<Layer>>::const_reverse_iterator crbegin() const { return layers_.crbegin(); }
        inline std::vector<Scope<Layer>>::const_reverse_iterator crend() const { return layers_.crend(); }

        Size size() const { return layers_.size(); }
    private:
        std::vector<Scope<Layer>> layers_{};
        uint32 insert_index_{};
    };
}
