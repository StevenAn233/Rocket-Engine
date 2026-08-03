module;
module LayerStack;

import Log;
import Layer;

namespace rke
{
    LayerStack::~LayerStack()
        { while(!layers_.empty()) pop_back(); }

    void LayerStack::push_layer(Scope<Layer> layer)
    {
        CORE_ASSERT(layer, u8"LayerStack: Layer null!");
        Layer& to_attach{ *layer };
        layer->layer_index_ = insert_index_;
        layers_.emplace(layers_.begin() + insert_index_, std::move(layer));
        to_attach.on_attach();
        insert_index_++;
    }

    void LayerStack::push_overlay(Scope<Layer> overlay)
    {
        CORE_ASSERT(overlay, u8"LayerStack: Overlay null!");
        Layer& to_attach{ *overlay };
        overlay->layer_index_ = this->size();
        layers_.push_back(std::move(overlay));
        to_attach.on_attach();
    }

    Scope<Layer> LayerStack::pop_layer()
    {
        if(!insert_index_) {
            CORE_ERROR(u8"LayerStack: Layers empty!");
            return nullptr;
        }
        auto it{ layers_.end() + insert_index_ - 1 };
        Scope<Layer> to_detach{ std::move(*it) };
        layers_.erase(it);
        to_detach->on_detach();
        return to_detach;
    }

    Scope<Layer> LayerStack::pop_overlay()
    {
        if(size() == insert_index_) {
            CORE_ERROR(u8"LayerStack: OverLays empty!");
            return nullptr;
        }
        return pop_back();
    }

    Scope<Layer> LayerStack::pop_back()
    {
        auto it{ layers_.end() - 1 };
        Scope<Layer> to_detach{ std::move(*it) };
        layers_.erase(it);
        to_detach->on_detach();
        return to_detach;
    }

    Layer& LayerStack::back() { return *(layers_.back().get()); }
}
