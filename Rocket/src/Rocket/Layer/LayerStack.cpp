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
        layer->on_attach();
        layer->layer_index_ = insert_index_;
        layers_.emplace(layers_.begin() + insert_index_, std::move(layer));
        insert_index_++;
    }

    void LayerStack::push_overlay(Scope<Layer> overlay)
    {
        overlay->on_attach();
        overlay->layer_index_ = this->size();
        layers_.push_back(std::move(overlay));
    }

    Scope<Layer> LayerStack::pop_layer()
    {
        if(!insert_index_) {
            CORE_ERROR(u8"LayerStack: Layers empty!");
            return nullptr;
        }
        auto it{ layers_.end() + insert_index_ - 1 };
        it->get()->on_detach();
        Scope<Layer> temp{ std::move(*it) };
        layers_.erase(it);
        return temp;
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
        it->get()->on_detach();
        Scope<Layer> temp{ std::move(*it) };
        layers_.erase(it);
        return temp;
    }

    Layer& LayerStack::back() { return *(layers_.back().get()); }
}
