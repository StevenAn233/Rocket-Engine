module;
module ConfigProxy;
import :Yaml;

import HeapManager;

namespace rke
{
    Scope<ConfigWriter> ConfigWriter::create()
        { return create_scope<YamlConfigWriter>(); }

    Scope<ConfigReader> ConfigReader::create(const Path& path)
        { return create_scope<YamlConfigReader>(path); }

    Scope<ConfigDocument> ConfigDocument::create(const Path& path)
        { return create_scope<YamlConfigDocument>(path); }
    Scope<ConfigDocument> ConfigDocument::create_map()
        { return create_scope<YamlConfigDocument>(true); }
    Scope<ConfigDocument> ConfigDocument::create_array()
        { return create_scope<YamlConfigDocument>(false); }
}
