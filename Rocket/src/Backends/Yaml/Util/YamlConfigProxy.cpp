module;

#include <yaml-cpp/yaml.h>

module ConfigProxy;
import :Yaml;

import Log;
import FileUtils;

namespace {
    using namespace rke;

    static ConfigValue read_impl(const YAML::Node& nd, const ConfigValue& def)
    {
        return std::visit([&nd]<typename T>(T&& val)-> ConfigValue
        {
            try {
                return ConfigValue(nd.as<std::decay_t<T>>());
            } catch(const std::exception& e) {
                CORE_ERROR(u8"YamlConfigReader: {}!", e.what());
                return ConfigValue(val);
            }
        }, def);
    }
}

namespace YAML
{
    template<>
    struct convert<glm::vec2>
    {
        static Node encode(glm::vec2 rhs)
        {
            Node node{};
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec2& rhs)
        {
            if(!node.IsSequence() || node.size() != 2) return false;
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec3>
    {
        static Node encode(glm::vec3 rhs)
        {
            Node node{};
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs)
        {
            if(!node.IsSequence() || node.size() != 3) return false;
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec4>
    {
        static Node encode(glm::vec4 rhs)
        {
            Node node{};
            node.push_back(rhs.r);
            node.push_back(rhs.g);
            node.push_back(rhs.b);
            node.push_back(rhs.a);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec4& rhs)
        {
            if(!node.IsSequence() || node.size() != 4) return false;
            rhs.r = node[0].as<float>();
            rhs.g = node[1].as<float>();
            rhs.b = node[2].as<float>();
            rhs.a = node[3].as<float>();
            return true;
        }
    };

    template<>
    struct convert<rke::String>
    {
        static Node encode(rke::String rhs)
        {
            Node node{};
            node.push_back(rhs.raw());
            return node;
        }

        static bool decode(const Node& node, rke::String& rhs)
        {
            std::string origin{ node.as<std::string>() };
            rhs = rke::String(rke::str::to_char8(origin.data()), origin.size());
            return true;
        }
    };

    static Emitter& operator<<(Emitter& yout, glm::vec2 v)
    {
        yout << Flow;
        yout << BeginSeq << v.x << v.y << EndSeq;
        return yout;
    }

    static Emitter& operator<<(Emitter& yout, glm::vec3 v)
    {
        yout << Flow;
        yout << BeginSeq << v.x << v.y << v.z << EndSeq;
        return yout;
    }

    static Emitter& operator<<(Emitter& yout, glm::vec4 v)
    {
        yout << Flow;
        yout << BeginSeq << v.r << v.g << v.b << v.a << EndSeq;
        return yout;
    }

    static Emitter& operator<<(Emitter& yout, const rke::String& str)
    {
        yout << str.raw();
        return yout;
    }
}

namespace rke
{
// YamlConfigWriter
    void YamlConfigWriter::begin_map(StringView name)
        { out_ << YAML::Key << name.raw_unsafe() << YAML::BeginMap; }
    void YamlConfigWriter::begin_map() { out_ << YAML::BeginMap; }
    void YamlConfigWriter::end_map() { out_ << YAML::EndMap; }

    void YamlConfigWriter::begin_array(StringView name)
        { out_ << YAML::Key << name.raw_unsafe() << YAML::BeginSeq; }
    void YamlConfigWriter::begin_array() { out_ << YAML::BeginSeq; }
    void YamlConfigWriter::end_array() { out_ << YAML::EndSeq; }

    void YamlConfigWriter::write(const ConfigValue& value)
    {
        try {
            std::visit([this]<typename T>(T&& arg)-> void { out_ << arg; }, value);
        } catch(const std::exception& e) {
            CORE_ASSERT(false, u8"YamlConfigWriter: {}!", e.what());
        }
    }

    void YamlConfigWriter::write(StringView key, const ConfigValue& value)
    {
        try {
            out_ << YAML::Key << key.raw_unsafe() << YAML::Value;
            std::visit([this]<typename T>(T&& arg)-> void { out_ << arg; }, value);
        } catch(const std::exception& e) {
            CORE_ASSERT(false, u8"YamlConfigWriter: {}!", e.what());
        }
    }

    bool YamlConfigWriter::push_to_file(const Path& path)
        { return file::write_file_string(path, out_.c_str()); }

// YamlConfigReader
    YamlConfigReader::YamlConfigReader(const Path& path)
    {
        std::ifstream fin(path.get());
        if(!fin) {
            CORE_ERROR(u8"YamlConfigReader: Failed to open YAML file: {}!", path);
            return;
        }
        node_ = YAML::Load(fin);
    }

    Size YamlConfigReader::size() const { return node_.size(); }
    bool YamlConfigReader::is_map() const { return node_.IsMap(); }
    bool YamlConfigReader::is_array() const { return node_.IsSequence(); }
        
    bool YamlConfigReader::has_key(StringView key) const
    {
        if(!node_.IsMap()) {
            CORE_ERROR(u8"YamlConfigReader: Not a map!");
            return false;
        }
        return node_[key.raw_unsafe()].IsDefined();
    }

    Scope<ConfigReader> YamlConfigReader::get_child(StringView name) const
    {
        if(!is_map()) {
            CORE_ERROR(u8"YamlConfigReader: Not a map!");
            return nullptr;
        }
        YAML::Node sub_node{ node_[name.raw_unsafe()] };
        return sub_node ? create_scope<YamlConfigReader>(sub_node) : nullptr;
    }
    Scope<ConfigReader> YamlConfigReader::get_child(Size index) const
    {
        if(!is_array()) {
            CORE_ERROR(u8"YamlConfigReader: Not an array!");
            return nullptr;
        }
        if(index < node_.size()) 
            return create_scope<YamlConfigReader>(node_[index]);
        CORE_ERROR(u8"YamlConfigReader: Out of bound!");
        return nullptr;
    }

    void YamlConfigReader::for_each_map(const MapCallback& callback) const
    {
        if(!node_.IsMap() || !callback) {
            CORE_ERROR(u8"YamlConfigReader: Not a map or callback null!");
            return;
        }
        for(auto it{ node_.begin() }; it != node_.end(); ++it) {
            String key_str{ it->first.as<String>() };
            callback(std::move(key_str), create_scope<YamlConfigReader>(it->second));
        }
    }

    void YamlConfigReader::for_each_arr(const SeqCallback& callback) const
    {
        if(!node_.IsSequence() || !callback) {
            CORE_ERROR(u8"YamlConfigReader: Not an array or callback null!");
            return;
        }
        for(Size i{}; i < node_.size(); ++i)
            callback(create_scope<YamlConfigReader>(node_[i]));
    }

    ConfigValue YamlConfigReader::read(StringView key, const ConfigValue& def) const
    {
        YAML::Node sub_node{ node_[key.raw_unsafe()] };
        if(!sub_node) {
            CORE_ERROR(u8"YamlConfigReader: Got nothing from key '{}'!", key);
            return ConfigValue(def);
        }
        return read_impl(sub_node, def);
    }

    ConfigValue YamlConfigReader::read(Size index, const ConfigValue& def) const
    {
        if(index >= node_.size()) {
            CORE_ERROR(u8"YamlConfigReader: Out of bound!");
            return ConfigValue(def);
        }
        YAML::Node sub_node{ node_[index] };
        return sub_node ? read_impl(sub_node, def) : ConfigValue(def);
    }

    ConfigValue YamlConfigReader::read_this(const ConfigValue& def) const
        { return read_impl(node_, def); }

// YamlConfigDucument
    YamlConfigDocument::YamlConfigDocument(const Path& path)
    {
        std::ifstream fin(path.get());
        if(!fin) {
            CORE_ERROR(u8"YamlConfigDocument: Failed to open YAML file: {}!", path);
            return;
        }
        node_ = YAML::Load(fin);
    }

    YamlConfigDocument::YamlConfigDocument(bool if_is_map) : node_(if_is_map ?
        YAML::Node(YAML::NodeType::Map) :
        YAML::Node(YAML::NodeType::Sequence)) {}

    Size YamlConfigDocument::size() const { return node_.size(); }
    bool YamlConfigDocument::is_map() const { return node_.IsMap(); }
    bool YamlConfigDocument::is_array() const { return node_.IsSequence(); }
        
    void YamlConfigDocument::push_back(const ConfigValue& value)
    {
        try {
            std::visit([this]<typename T>(T&& arg)
                { node_.push_back(arg); }, value);
        } catch(const std::exception& e) {
            CORE_ASSERT(false, u8"YamlConfigDocument: {}!", e.what());
        }
    }

    Scope<ConfigDocument> YamlConfigDocument::push_map()
    {
        try {
            node_.push_back(YAML::Node(YAML::NodeType::Map));
            return create_scope<YamlConfigDocument>(node_[node_.size() - 1]);
        } catch(const std::exception& e) {
            CORE_ERROR(u8"YamlConfigDocument: {}!", e.what());
            return nullptr;
        }
    }

    Scope<ConfigDocument> YamlConfigDocument::push_array()
    {
        try {
            node_.push_back(YAML::Node(YAML::NodeType::Sequence));
            return create_scope<YamlConfigDocument>(node_[node_.size() - 1]);
        } catch(const std::exception& e) {
            CORE_ERROR(u8"YamlConfigDocument: {}!", e.what());
            return nullptr;
        }
    }

    void YamlConfigDocument::set(Size index, const ConfigValue& value)
    {
        try {
            if(index < node_.size())
                std::visit([this, index]<typename T>(T&& arg)
                    { node_[index] = arg; }, value);
            CORE_ERROR(u8"YamlConfigDocument: Out of bound!");
        } catch(const std::exception& e) {
            CORE_ASSERT(false, u8"YamlConfigDocument: {}!", e.what());
        }
    }

    void YamlConfigDocument::write(StringView key, const ConfigValue& value)
    {
        try {
            std::visit([this, key]<typename T>(T&& arg)
                { node_[key.raw_unsafe()] = arg; }, value);
        } catch(const std::exception& e) {
            CORE_ASSERT(false, u8"YamlConfigDocument: {}!", e.what());
        }
    }

    Scope<ConfigDocument> YamlConfigDocument::get_child(StringView key)
    {
        if(!is_map()) {
            CORE_ERROR(u8"YamlConfigDocument: Not a map!");
            return nullptr;
        }
        return create_scope<YamlConfigDocument>(node_[key.raw_unsafe()]);
    }
    // will create a node(pair) if doesn't exist

    Scope<ConfigDocument> YamlConfigDocument::get_child(Size index)
    {
        if(!is_array()) {
            CORE_ERROR(u8"YamlConfigDocument: Not an array!");
            return nullptr;
        }
        if(index < node_.size())
            return create_scope<YamlConfigDocument>(node_[index]);
        CORE_ERROR(u8"YamlConfigDocument: Out of bound!");
        return nullptr;
    }

    void YamlConfigDocument::push_to_file(const Path& path)
    {
        YAML::Emitter out{}; out << node_;
        file::write_file_string(path, out.c_str());
    }
}
