module;
module LayerStack;

import Log;
import Layer;

namespace rke
{
    LayerStack::~LayerStack() { for(auto& layer : layers_) layer->on_detach(); }

    void LayerStack::push_layer(Scope<Layer> layer)
    {
        layer->on_attach();
        layer->layer_index_ = insert_index_;
        layers_.emplace(layers_.begin() + insert_index_, std::move(layer));
        insert_index_++;
    }

    Scope<Layer> LayerStack::pop_layer(Layer* layer)
    {
        auto central{ layers_.begin() + insert_index_ };
        auto it{ std::find_if(layers_.begin(), central,
            [layer](const Scope<Layer>& item) { return item.get() == layer; }) };
        if(it != central /* Layer is found */)
        {
            it->get()->on_detach();
            Scope<Layer> temp{ std::move(*it) };
            layers_.erase(it);
            insert_index_--;
            return temp;
        }
        else CORE_WARN(u8"LayerStack: Layer not found!");
        return nullptr;
    }

    void LayerStack::push_overlay(Scope<Layer> overlay)
    {
        overlay->on_attach();
        overlay->layer_index_ = this->size();
        layers_.push_back(std::move(overlay));
    }

    Scope<Layer> LayerStack::pop_overlay(Layer* overlay)
    {
        auto central{ layers_.begin() + insert_index_ };
        auto it{ std::find_if(central, layers_.end(),
            [overlay](const Scope<Layer>& item) { return item.get() == overlay; }) };
        if(it != layers_.end() /* Overlay is found */)
        {
            it->get()->on_detach();
            Scope<Layer> temp{ std::move(*it) };
            layers_.erase(it);
            return temp;
        }
        else CORE_WARN(u8"LayerStack: overlay not found!");
        return nullptr;
    }
}
