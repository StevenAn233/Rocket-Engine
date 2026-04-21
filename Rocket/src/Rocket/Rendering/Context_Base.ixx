module;

#include <memory>

export module Context:Base;

import NativeWindow;
import HeapManager;

export namespace rke
{
    class Context
    {
    public:
        friend struct std::default_delete<Context>;

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context&&) = delete;

        virtual void init() = 0;
        virtual void swap_buffers() = 0;

        static Scope<Context> create(NativeWindow handle);
    protected:
        Context() = default;
        virtual ~Context() = default;
    };
}
