module;

// required
#include <string>
#include <string_view>
#include <format>

#include <functional>
#include "rke_macros.h"

export module String;

import Types;

export namespace rke
{
    class RKE_API StringView final
    {
    public:
        constexpr StringView() = default;
        constexpr StringView(const StringView&) noexcept = default;
        template<Size N>
        constexpr StringView(const char8 (&str)[N]) : view_(str, N - 1) {}
        constexpr StringView(const char8* data, Size size) : view_(data, size) {}

        explicit constexpr StringView(const char8* str) : view_(str) {}

        constexpr StringView& operator=(const StringView&) noexcept = default;

    //  Doesn't guarantee that it ends with '\0'
        const char* raw_unsafe() const { return reinterpret_cast<const char*>(view_.data()); }
        constexpr const char8* data() const { return view_.data(); }

        constexpr Size size () const { return view_.size (); }
        constexpr bool empty() const { return view_.empty(); }
        
        constexpr auto begin() const { return view_.begin(); }
        constexpr auto end  () const { return view_.end  (); }

        constexpr bool ends_with(const char8* s) const { return view_.ends_with(s); }

        constexpr bool operator==(const char8* s) const { return view_ == s; }
        constexpr bool operator!=(const char8* s) const { return view_ != s; }
        constexpr bool operator< (const char8* s) const { return view_ <  s; }
        constexpr bool operator<=(const char8* s) const { return view_ <= s; }
        constexpr bool operator> (const char8* s) const { return view_ >  s; }
        constexpr bool operator>=(const char8* s) const { return view_ >= s; }

        constexpr bool operator==(StringView other) const { return view_ == other.view_; }
        constexpr bool operator!=(StringView other) const { return view_ != other.view_; }
        constexpr bool operator< (StringView other) const { return view_ <  other.view_; }
        constexpr bool operator<=(StringView other) const { return view_ <= other.view_; }
        constexpr bool operator> (StringView other) const { return view_ >  other.view_; }
        constexpr bool operator>=(StringView other) const { return view_ >= other.view_; }

        constexpr Size find_last_of(const char8* target, Size offset = npos) const
            { return view_.find_last_of(target, offset); }
        constexpr StringView substr(Size offset = 0, Size count = npos) const
            { return view_.substr(offset, count); }

    // additional
        bool is_null_terminated() const // '\0' check
        {
            if(view_.data()) {
                const char8* end{ view_.data() + view_.size() };
                return *end == u8'\0';
            }
            return true;
        }
    public:
        static constexpr Size npos{ std::u8string_view::npos };
    private:
        constexpr StringView(std::u8string_view view) noexcept : view_(view) {}
    private:
        std::u8string_view view_{};
    };

    template<typename... Args>
    struct U8FormatString
    {
        const char8* c_str;

        template<Size N>
        consteval U8FormatString(const char8 (&fmt)[N]) : c_str(fmt) 
        {
            char check_buf[N]{};
            for(Size i{}; i < N; ++i)
                check_buf[i] = static_cast<char>(fmt[i]);

            (void)std::format_string<Args...>(check_buf);
        }
    };

    class RKE_API String final
    {
    public:
        friend struct std::hash<String>;

        constexpr String()  = default;
        constexpr ~String() = default;
        constexpr String(const String& other) noexcept : u8string_(other.u8string_) {};
        constexpr String(String&& other) noexcept : u8string_(std::move(other.u8string_)) {};
        constexpr String(const std::u8string& s) : u8string_(s) {}
        constexpr String(std::u8string&& s) : u8string_(std::move(s)) {}
        template<Size N>
        constexpr String(const char8 (&str)[N]) : u8string_(str, N - 1) {}
        constexpr String(const char8* s, Size len)  : u8string_(s, len) {}
        constexpr String(StringView sv) : u8string_(sv.data(), sv.size()) {}
        explicit constexpr String(const char8* s) : u8string_(s) {}

        constexpr String& operator=(const String& other) noexcept
            { u8string_ = other.u8string_; return *this; }
        constexpr String& operator=(String&& other) noexcept
            { u8string_ = std::move(other.u8string_); return *this; }
        template<Size N>
        constexpr String& operator=(const char8 (&s)[N])
            { u8string_ = std::u8string(s); return *this; }

        constexpr operator StringView() const
            { return StringView(u8string_.data(), u8string_.size()); }

        const char* raw() const /*IMPORTANT*/
            { return reinterpret_cast<const char*>(u8string_.c_str()); }
        // the whole project is based on utf-8 so this is absolutely fine
        const std::u8string& get() const { return u8string_; }
        
    // standard
        constexpr const char8* c_str() const { return u8string_.c_str(); }
        constexpr const char8* data () const { return u8string_.data (); }
        constexpr Size size  () const { return u8string_.size  (); }
        constexpr Size length() const { return u8string_.length(); }
        constexpr bool empty () const { return u8string_.empty (); }

        constexpr bool ends_with(const char8* s) const { return u8string_.ends_with(s); }

        constexpr String operator+(const char8* s) const { return String(u8string_ + s); }
        constexpr String operator+(char8 ch) const { return String(u8string_ + ch); }
        constexpr String operator+(const String& other) const
            { return String(u8string_ + other.u8string_); }
        
        constexpr String& operator+=(const char8* s) { u8string_ += s; return *this; }
        constexpr String& operator+=(char8 ch) { u8string_ += ch; return *this; }
        constexpr String& operator+=(const String& other)
            { u8string_ += other.u8string_; return *this; }

        constexpr bool operator==(const String& other) const { return u8string_ == other.u8string_; }
        constexpr bool operator!=(const String& other) const { return u8string_ != other.u8string_; }
        constexpr bool operator< (const String& other) const { return u8string_ <  other.u8string_; }
        constexpr bool operator<=(const String& other) const { return u8string_ <= other.u8string_; }
        constexpr bool operator> (const String& other) const { return u8string_ >  other.u8string_; }
        constexpr bool operator>=(const String& other) const { return u8string_ >= other.u8string_; }

        constexpr bool operator==(const char8* s) const { return u8string_ == s; }
        constexpr bool operator!=(const char8* s) const { return u8string_ != s; }
        constexpr bool operator< (const char8* s) const { return u8string_ <  s; }
        constexpr bool operator<=(const char8* s) const { return u8string_ <= s; }
        constexpr bool operator> (const char8* s) const { return u8string_ >  s; }
        constexpr bool operator>=(const char8* s) const { return u8string_ >= s; }

        constexpr bool operator==(StringView sv) const { return StringView(*this) == sv; }
        constexpr bool operator!=(StringView sv) const { return StringView(*this) != sv; }
        constexpr bool operator< (StringView sv) const { return StringView(*this) <  sv; }
        constexpr bool operator<=(StringView sv) const { return StringView(*this) <= sv; }
        constexpr bool operator> (StringView sv) const { return StringView(*this) >  sv; }
        constexpr bool operator>=(StringView sv) const { return StringView(*this) >= sv; }

        constexpr char8& operator[](Size index) { return u8string_[index]; }
        constexpr const char8& operator[](Size index) const { return u8string_[index]; }
        constexpr const char8& at(Size index) const { return u8string_.at(index); }

        constexpr auto begin() { return u8string_.begin(); }
        constexpr auto end  () { return u8string_.end  (); }
        constexpr auto begin() const { return u8string_.begin(); }
        constexpr auto end  () const { return u8string_.end  (); }
        constexpr auto cbegin() const { return u8string_.cbegin(); }
        constexpr auto cend  () const { return u8string_.cend  (); }
        constexpr auto rbegin() { return u8string_.rbegin(); }
        constexpr auto rend  () { return u8string_.rend  (); }
        constexpr auto crbegin() const { return u8string_.crbegin(); }
        constexpr auto crend  () const { return u8string_.crend  (); }

        constexpr Size find(const String& target, Size offset = 0) const
            { return u8string_.find(target.u8string_, offset); }
        constexpr Size find(char8 ch) const { return u8string_.find(ch); }

        constexpr bool contains(const String& substr) const { return find(substr) != npos; }
        constexpr bool contains(char8 ch) const { return find(ch) != npos; }

        constexpr String substr(Size offset = 0, Size count = npos) const
            { return String(u8string_.substr(offset, count)); }

        constexpr String& replace(Size offset, Size len, const String& obj)
        {
            u8string_.replace(offset, len, obj.u8string_);
            return *this;
        }

        constexpr void clear() noexcept { u8string_.clear(); }
        constexpr void swap(String& other) noexcept { u8string_.swap(other.u8string_); }

    // additional
        void replace_search_by(const String& search, const String& replace_obj)
        {
            Size pos{};
            while((pos = find(search, pos)) != String::npos)
            {
                replace(pos, search.length(), replace_obj);
                pos += replace_obj.length();
            }
        }

        template<typename... Args>
        static String format(U8FormatString<std::type_identity_t<Args>...> fmt, Args&&... args)
        {
            std::string_view fmt_sv(reinterpret_cast<const char*>(fmt.c_str));
            auto fmt_args{ std::make_format_args(args...) };
            CharBuffer temp{ std::vformat(fmt_sv, fmt_args) }; // do allocate memory(heap/stack)
            return String(reinterpret_cast<const char8*>(temp.data()), temp.size());
        }
    public:
        static constexpr Size npos{ std::u8string::npos };
    private:
        std::u8string u8string_{};
    };
}

namespace std
{
    export template<>
    struct hash<rke::String> {
        size_t operator()(const rke::String& s) const
            { return hash<u8string>()(s.u8string_); }
    };

    export template<>
    struct formatter<rke::StringView> : formatter<string_view>
    {
        auto format(rke::StringView u8sv, format_context& ctx) const
        {
            string_view sv(u8sv.raw_unsafe(), u8sv.size());
            return formatter<string_view>::format(sv, ctx);
        }
    };

    export template<>
    struct formatter<rke::String> : formatter<string_view>
    {
        auto format(const rke::String& str, format_context& ctx) const
        {
            string_view sv(str.raw(), str.size());
            return formatter<string_view>::format(sv, ctx);
        }
    };
}

export namespace rke::literals
{
    constexpr StringView operator""_sv(const char8* str, Size len)
        { return StringView(str, len); }
    inline String operator""_s(const char8* str, Size len)
        { return String(str, len); }
}

export namespace rke::str
{
    inline const char8* to_char8(const char* s)
    {
        // check the string somehow maybe?
        return reinterpret_cast<const char8*>(s);
    }

    // caller should make sure the life-time of the original path
    constexpr StringView extract_filename(StringView path)
    {
        auto pos{ path.find_last_of(u8"/\\") };
        if(pos == StringView::npos) return path;
        return path.substr(pos + 1); 
    }

    inline void replace_search_by(String& origin,
        const String& search, const String& target)
    {
        Size pos{};
        while((pos = origin.find(search, pos)) != String::npos)
        {
            origin.replace(pos, search.length(), target);
            pos += target.length();
        }
    }

    inline void replace_search_by(CharBuffer& origin,
        const CharBuffer& search, const CharBuffer& target)
    {
        Size pos{};
        while((pos = origin.find(search, pos)) != String::npos)
        {
            origin.replace(pos, search.length(), target);
            pos += target.length();
        }
    }
}
