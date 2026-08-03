module;
module PostProcessEffect;

namespace rke
{
    PostProcessEffect::PostProcessEffect(String name, std::function<bool()> func)
        : name_(name.empty() ? u8"Untitled" : std::move(name))
        , enabled_situation_(func ? std::move(func) : []() { return true; }) {}
}
