module;

#include <yaml-cpp/yaml.h>

export module ConfigProxy:Yaml;
import :Base;

import Types;
import String;

namespace rke
{
    class YamlConfigWriter : public ConfigWriter
    {
    public:
        YamlConfigWriter() = default;

        void begin_map(StringView name) override;
        void begin_map() override;
        void end_map() override;

        void begin_array(StringView name) override;
        void begin_array() override;
        void end_array() override;

        void write(const ConfigValue& value) override;
        void write(StringView key, const ConfigValue& value) override;

        bool push_to_file(const Path& path) override;
    private:
        YAML::Emitter out_{};
    };

    class YamlConfigReader : public ConfigReader
    {
    public:
        YamlConfigReader(const Path& path);
        YamlConfigReader(const YAML::Node& nd) : node_(nd) {}

        Size size() const override;
        bool is_map() const override;
        bool is_array() const override;
        bool has_key(StringView key) const override;

        Scope<ConfigReader> get_child(StringView name) const override;
        Scope<ConfigReader> get_child(Size index) const override;

        void for_each_map(const MapCallback& callback) const override;
        void for_each_arr(const SeqCallback& callback) const override;
    protected:
        ConfigValue read(StringView key, const ConfigValue& def) const override;
        ConfigValue read(Size index, const ConfigValue& def) const override;
        ConfigValue read_this(const ConfigValue& def) const override;
    private:
        YAML::Node node_{};
    };

    class YamlConfigDocument : public ConfigDocument
    {
    public:
        YamlConfigDocument(const Path& path);
        YamlConfigDocument(bool if_is_map);
        YamlConfigDocument(const YAML::Node& node) : node_(node) {}

        Size size() const override;
        bool is_map() const override;
        bool is_array() const override;
        
        void push_back(const ConfigValue& value) override;
        Scope<ConfigDocument> push_map() override;
        Scope<ConfigDocument> push_array() override;

        void set(Size index, const ConfigValue& value) override;
        void write(StringView key, const ConfigValue& value) override;

        Scope<ConfigDocument> get_child(StringView key) override;
        Scope<ConfigDocument> get_child(Size index) override;

        void push_to_file(const Path& path) override;
    private:
        YAML::Node node_{};
    };
}
