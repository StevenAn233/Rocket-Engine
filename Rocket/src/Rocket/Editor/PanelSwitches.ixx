module;

#include <map>
#include "rke_macros.h"

export module PanelSwitches;

import String;
import Path;

export namespace rke
{
    class RKE_API PanelSwitches
    {
    public:
        ~PanelSwitches();

        bool& operator[](const String& key) { return switches_[key]; }
        void check_to_add(const String& key, bool value = true);
        void check_to_remove(const String& key)
        {
            auto it{ switches_.find(key) };
            if(it != switches_.end()) switches_.erase(it);
        }

        bool has(const String& key) const;
        bool at (const String& key) const;

        auto begin() { return switches_.begin(); }
        auto end  () { return switches_.end  (); }
        auto begin() const { return switches_.begin(); }
        auto end  () const { return switches_.end  (); }

        void load_from(Path filepath);
    private:
        std::map<String, bool> switches_{}; // fixed-order
        Path filepath_{};
    };
}
