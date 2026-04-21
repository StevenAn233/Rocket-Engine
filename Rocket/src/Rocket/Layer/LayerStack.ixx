module;

#include <vector>

export module LayerStack;

import Types;
import Layer;
import HeapManager;

export namespace rke
{
    class LayerStack
    {
    public:
        LayerStack() = default;
        ~LayerStack() { for(auto& layer : layers_) layer->on_detach(); }

        void push_layer   (Scope<Layer> layer  ); // put layer to the first-half
        void push_overlay (Scope<Layer> overlay); // put layer to the second-half
        Scope<Layer> pop_layer  (Layer* layer  ); // pop specified first-half-layer
        Scope<Layer> pop_overlay(Layer* overlay); // pop specified second-half-layer

        std::vector<Scope<Layer>>::iterator begin() { return layers_.begin();  }
        std::vector<Scope<Layer>>::iterator end  () { return layers_.end();    }
        std::vector<Scope<Layer>>::reverse_iterator rbegin() { return layers_.rbegin(); }
        std::vector<Scope<Layer>>::reverse_iterator rend  () { return layers_.rend();   }

        std::vector<Scope<Layer>>::const_iterator cbegin() const { return layers_.cbegin();  }
        std::vector<Scope<Layer>>::const_iterator cend  () const { return layers_.cend();    }
        std::vector<Scope<Layer>>::const_reverse_iterator crbegin() const { return layers_.crbegin(); }
        std::vector<Scope<Layer>>::const_reverse_iterator crend  () const { return layers_.crend();   }

        Size size() const { return layers_.size(); }
    private:
        std::vector<Scope<Layer>> layers_{};
        uint32 insert_index_{};
    };
}
