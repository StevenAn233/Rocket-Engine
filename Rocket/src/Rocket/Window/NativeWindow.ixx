module;

#include <bit>

export module NativeWindow;

import Types;

export namespace rke
{
    class NativeWindow
    {
    public:
        NativeWindow() = default;
        template<typename T>
        explicit NativeWindow(T* ptr) : handle_(static_cast<void*>(ptr)) {}

        template<typename T>
        T* as() const { return static_cast<T*>(handle_); }

        void* get() const { return handle_; }
        uintptr get_integral() const { return std::bit_cast<uintptr>(handle_); }
        
        explicit operator bool() const { return handle_ != nullptr; }
        bool operator==(const NativeWindow& other) const { return handle_ == other.handle_; }
    private:
        void* handle_{ nullptr };
    };
}
