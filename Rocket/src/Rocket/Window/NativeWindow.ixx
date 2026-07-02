export module NativeWindow;

import Types;

export namespace rke
{
    class NativeWindow
    {
    public:
        explicit NativeWindow(void* window = nullptr) : ctx_(window) {}

        template<typename T>
        T* as() const { return static_cast<T*>(ctx_); }

        inline void* get() const { return ctx_; }
        inline uintptr val() const { return reinterpret_cast<uintptr>(ctx_); }
        
        explicit operator bool() const { return ctx_ != nullptr; }
        bool operator==(const NativeWindow& other) const { return ctx_ == other.ctx_; }
    private:
        void* ctx_;
    };
}
