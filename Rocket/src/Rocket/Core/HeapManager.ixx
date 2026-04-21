module;

#include <memory>

export module HeapManager;

export namespace rke // to be self-implemented
{
// Ref(shared)
    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    Ref<T> create_ref(Args&&... args)
        { return std::make_shared<T>(std::forward<Args>(args)...); }

    template<typename T>
    consteval Ref<T> null_ref() { return nullptr; }

// Scope(unique)
    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T, typename... Args>
    Scope<T> create_scope(Args&&... args)
        { return std::make_unique<T>(std::forward<Args>(args)...); }

    template<typename T>
    consteval Scope<T> null_scope() { return nullptr; }
}
