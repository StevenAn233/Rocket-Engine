module;

#include <variant>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module ConfigProxy:Base;

import Types;
import String;
import Path;
import HeapManager;

export namespace rke
{
    using ConfigValue = std::variant <
        int16, uint16, int32, uint32, int64, uint64, 
        bool, float, double, String,
        glm::vec2, glm::vec3, glm::vec4
    >;

    class RKE_API ConfigWriter
    {
    public:
        virtual ~ConfigWriter() = default;

        virtual void begin_map(StringView name) = 0;
        virtual void begin_map() = 0;
        virtual void end_map  () = 0;

        virtual void begin_array(StringView name) = 0;
        virtual void begin_array() = 0;
        virtual void end_array  () = 0;

        virtual void write(StringView key, const ConfigValue& value) = 0;
        virtual void write(const ConfigValue& value) = 0;

        virtual bool push_to_file(const Path& path) = 0;

        static Scope<ConfigWriter> create();
    };

    class RKE_API ConfigReader
    {
    public:
        virtual ~ConfigReader() = default;

        virtual Size size() const = 0;
        virtual bool is_map() const = 0;
        virtual bool is_array() const = 0;
        virtual bool has_key(StringView key) const = 0;
        
        virtual Scope<ConfigReader> get_child(StringView name) const = 0;
        virtual Scope<ConfigReader> get_child(Size index) const = 0;

        using MapCallback = std::function<void(String, Scope<ConfigReader>)>;
        using SeqCallback = std::function<void(Scope<ConfigReader>)>;
        virtual void for_each_map(const MapCallback& callback) const = 0;
        virtual void for_each_arr(const SeqCallback& callback) const = 0;

        template<typename T>
        T get_at(StringView key, T def) const { return std::get<T>(read(key, ConfigValue(def))); }
        template<typename T>
        T get_at(Size index, T def) const { return std::get<T>(read(index, ConfigValue(def))); }
        template<typename T>
        T as(T def) const { return std::get<T>(read_this(ConfigValue(def))); }

        static Scope<ConfigReader> create(const Path& path);
    protected:
        virtual ConfigValue read(StringView key, const ConfigValue& def) const = 0;
        virtual ConfigValue read(Size index, const ConfigValue& def) const = 0;
        virtual ConfigValue read_this(const ConfigValue& def) const = 0;
    };

    class RKE_API ConfigDocument
    {
    public:
        virtual ~ConfigDocument() = default;

        virtual Size size() const = 0;
        virtual bool is_map() const = 0;
        virtual bool is_array() const = 0;

        // only for arrays
        virtual void push_back(const ConfigValue& value) = 0;
        virtual Scope<ConfigDocument> push_map() = 0;
        virtual Scope<ConfigDocument> push_array() = 0;
        virtual void set(Size index, const ConfigValue& value) = 0;

        virtual void write(StringView key, const ConfigValue& value) = 0;

        virtual Scope<ConfigDocument> get_child(StringView key) = 0;
        virtual Scope<ConfigDocument> get_child(Size index) = 0;

        virtual void push_to_file(const Path& path) = 0;

        static Scope<ConfigDocument> create(const Path& path);
        static Scope<ConfigDocument> create_map();
        static Scope<ConfigDocument> create_array();
    };
}
