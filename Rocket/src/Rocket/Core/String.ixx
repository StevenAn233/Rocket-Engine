module;

// required
#include <string>
#include <string_view>
#include <format>
#include <algorithm>
#include <functional>
#include "rke_macros.h"

export module String;

import Types;

export namespace rke
{
    template<Size N>
    struct StringLiteral
    {
        char8 data[N];
        consteval StringLiteral(const char8 (&str)[N])
            { std::copy_n(str, N, data); }
    };

    class RKE_API StringView final
    {
    public:
        template<Size N>
        inline constexpr StringView(const char8 (&str)[N]) : view_(str, N - 1) {}
        inline constexpr StringView(const char8* data, Size size) : view_(data, size) {}
        inline explicit constexpr StringView(const char8* str) : view_(str) {}

        constexpr StringView(const StringView&) noexcept = default;
        constexpr StringView& operator=(const StringView&) noexcept = default;

    //  Doesn't guarantee that it ends with '\0'
        inline const char* raw_unsafe() const { return reinterpret_cast<const char*>(view_.data()); }
        inline constexpr const char8* data() const { return view_.data(); }

        inline constexpr Size size () const { return view_.size (); }
        inline constexpr bool empty() const { return view_.empty(); }
        
        inline constexpr auto begin() const { return view_.begin(); }
        inline constexpr auto end  () const { return view_.end  (); }

        inline constexpr bool ends_with(const char8* s) const { return view_.ends_with(s); }

        inline constexpr bool operator==(const char8* s) const { return view_ == s; }
        inline constexpr bool operator!=(const char8* s) const { return view_ != s; }
        inline constexpr bool operator< (const char8* s) const { return view_ <  s; }
        inline constexpr bool operator<=(const char8* s) const { return view_ <= s; }
        inline constexpr bool operator> (const char8* s) const { return view_ >  s; }
        inline constexpr bool operator>=(const char8* s) const { return view_ >= s; }

        inline constexpr bool operator==(StringView other) const { return view_ == other.view_; }
        inline constexpr bool operator!=(StringView other) const { return view_ != other.view_; }
        inline constexpr bool operator< (StringView other) const { return view_ <  other.view_; }
        inline constexpr bool operator<=(StringView other) const { return view_ <= other.view_; }
        inline constexpr bool operator> (StringView other) const { return view_ >  other.view_; }
        inline constexpr bool operator>=(StringView other) const { return view_ >= other.view_; }

        inline constexpr Size find_last_of(const char8* target, Size offset = npos) const
            { return view_.find_last_of(target, offset); }
        inline constexpr StringView substr(Size offset = 0, Size count = npos) const
            { return view_.substr(offset, count); }

    // additional
        inline bool is_null_terminated() const // '\0' check
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
        inline constexpr StringView(std::u8string_view view) noexcept : view_(view) {}
    private:
        std::u8string_view view_;
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

        inline constexpr String() : u8string_({}) {};
        inline constexpr ~String() {};
        inline constexpr String(const String& other) noexcept : u8string_(other.u8string_) {};
        inline constexpr String(String&& other) noexcept : u8string_(std::move(other.u8string_)) {};
        inline constexpr String(const std::u8string& s) : u8string_(s) {}
        inline constexpr String(std::u8string&& s) : u8string_(std::move(s)) {}
        template<Size N>
        inline constexpr String(const char8 (&str)[N]) : u8string_(str, N - 1) {}
        inline constexpr String(const char8* s, Size len) : u8string_(s, len) {}
        inline constexpr String(StringView sv) : u8string_(sv.data(), sv.size()) {}
        inline explicit constexpr String(const char8* s) : u8string_(s) {}

        inline constexpr String& operator=(const String& other) noexcept
            { u8string_ = other.u8string_; return *this; }
        inline constexpr String& operator=(String&& other) noexcept
            { u8string_ = std::move(other.u8string_); return *this; }
        template<Size N>
        inline constexpr String& operator=(const char8 (&s)[N])
            { u8string_ = std::u8string(s); return *this; }

        inline constexpr operator StringView() const
            { return StringView(u8string_.data(), u8string_.size()); }

        inline const char* raw() const /*IMPORTANT*/
            { return reinterpret_cast<const char*>(u8string_.c_str()); }
        // the whole project is based on utf-8 so this is absolutely fine
        inline const std::u8string& get() const { return u8string_; }
        
    // standard
        inline constexpr const char8* c_str() const { return u8string_.c_str(); }
        inline constexpr const char8* data () const { return u8string_.data (); }
        inline constexpr Size size  () const { return u8string_.size  (); }
        inline constexpr Size length() const { return u8string_.length(); }
        inline constexpr bool empty () const { return u8string_.empty (); }

        inline constexpr bool ends_with(const char8* s) const { return u8string_.ends_with(s); }

        inline constexpr String operator+(const char8* s) const { return String(u8string_ + s); }
        inline constexpr String operator+(char8 ch) const { return String(u8string_ + ch); }
        inline constexpr String operator+(const String& other) const
            { return String(u8string_ + other.u8string_); }
        
        inline constexpr String& operator+=(const char8* s) { u8string_ += s; return *this; }
        inline constexpr String& operator+=(char8 ch) { u8string_ += ch; return *this; }
        inline constexpr String& operator+=(const String& other)
            { u8string_ += other.u8string_; return *this; }

        inline constexpr bool operator==(const String& other) const { return u8string_ == other.u8string_; }
        inline constexpr bool operator!=(const String& other) const { return u8string_ != other.u8string_; }
        inline constexpr bool operator< (const String& other) const { return u8string_ <  other.u8string_; }
        inline constexpr bool operator<=(const String& other) const { return u8string_ <= other.u8string_; }
        inline constexpr bool operator> (const String& other) const { return u8string_ >  other.u8string_; }
        inline constexpr bool operator>=(const String& other) const { return u8string_ >= other.u8string_; }

        inline constexpr bool operator==(const char8* s) const { return u8string_ == s; }
        inline constexpr bool operator!=(const char8* s) const { return u8string_ != s; }
        inline constexpr bool operator< (const char8* s) const { return u8string_ <  s; }
        inline constexpr bool operator<=(const char8* s) const { return u8string_ <= s; }
        inline constexpr bool operator> (const char8* s) const { return u8string_ >  s; }
        inline constexpr bool operator>=(const char8* s) const { return u8string_ >= s; }

        inline constexpr bool operator==(StringView sv) const { return StringView(*this) == sv; }
        inline constexpr bool operator!=(StringView sv) const { return StringView(*this) != sv; }
        inline constexpr bool operator< (StringView sv) const { return StringView(*this) <  sv; }
        inline constexpr bool operator<=(StringView sv) const { return StringView(*this) <= sv; }
        inline constexpr bool operator> (StringView sv) const { return StringView(*this) >  sv; }
        inline constexpr bool operator>=(StringView sv) const { return StringView(*this) >= sv; }

        inline constexpr char8& operator[](Size index) { return u8string_[index]; }
        inline constexpr const char8& operator[](Size index) const { return u8string_[index]; }
        inline constexpr const char8& at(Size index) const { return u8string_.at(index); }

        inline constexpr auto begin() { return u8string_.begin(); }
        inline constexpr auto end  () { return u8string_.end  (); }
        inline constexpr auto begin() const { return u8string_.begin(); }
        inline constexpr auto end  () const { return u8string_.end  (); }
        inline constexpr auto cbegin() const { return u8string_.cbegin(); }
        inline constexpr auto cend  () const { return u8string_.cend  (); }
        inline constexpr auto rbegin() { return u8string_.rbegin(); }
        inline constexpr auto rend  () { return u8string_.rend  (); }
        inline constexpr auto crbegin() const { return u8string_.crbegin(); }
        inline constexpr auto crend  () const { return u8string_.crend  (); }

        inline constexpr Size find(const String& target, Size offset = 0) const
            { return u8string_.find(target.u8string_, offset); }
        inline constexpr Size find(char8 ch) const { return u8string_.find(ch); }

        inline constexpr bool contains(const String& substr) const { return find(substr) != npos; }
        inline constexpr bool contains(char8 ch) const { return find(ch) != npos; }

        inline constexpr String substr(Size offset = 0, Size count = npos) const
            { return String(u8string_.substr(offset, count)); }

        inline constexpr String& replace(Size offset, Size len, const String& obj)
        {
            u8string_.replace(offset, len, obj.u8string_);
            return *this;
        }

        inline constexpr void clear() noexcept { u8string_.clear(); }
        inline constexpr void swap(String& other) noexcept { u8string_.swap(other.u8string_); }

    // additional
        inline void replace_search_by(const String& search, const String& replace_obj)
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
        std::u8string u8string_;
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
    inline constexpr StringView operator""_sv(const char8* str, Size len)
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
    inline constexpr StringView extract_filename(StringView path)
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
