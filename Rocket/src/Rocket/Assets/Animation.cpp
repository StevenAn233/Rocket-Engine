module;
module Animation;

import Log;
import FileUtils;
import AssetsManager;
import Application;

namespace rke
{
    Animation::Animation() : tex_uuid_(UUID(0)) {}

    bool Animation::load_from(const Path& filepath)
    {
        if(!filepath.exists()) {
            CORE_ERROR(u8"Animation: File '{}' doesn't exist!", filepath);
            return false;
        }
        if(!filepath.string().ends_with(u8".rkanim")) {
            CORE_ERROR(u8"Animation: '{}' isn't a .rkanim file!", filepath);
            return false;
        }

        Scope<ConfigReader> reader{ ConfigReader::create(filepath) };
        if(!reader || !reader->is_map()) {
            CORE_ERROR(u8"Animation: Failed to read '{}'!", filepath);
            return false;
        }

        Scope<ConfigReader> clips_reader{ reader->get_child(u8"Clips") };
        if(!clips_reader || !clips_reader->is_array()) {
            CORE_WARN(u8"Animation: No 'Clips' section in '{}'.", filepath);
            return false;
        }

        tex_uuid_ = AssetUUID(reader->get_at(u8"Texture", 0ui64));

        clips_.clear();
        clip_names_.clear();
        clips_reader->for_each([this](Scope<ConfigReader> clip_node)
        {
            String name{ clip_node->get_at(u8"Name", String{}) };
            AnimClip clip{};
            clip.cell_size = clip_node->get_at(u8"Cell Size", std::pair<int, int>(1, 1));
            clip.fps  = clip_node->get_at(u8"FPS", 12.0f);
            clip.loop = clip_node->get_at(u8"Loop", true);
            clip.next = clip_node->get_at(u8"Next", String{});

            Scope<ConfigReader> frames_reader{ clip_node->get_child(u8"Frames") };
            if(frames_reader && frames_reader->is_array())
            {
                frames_reader->for_each([&clip](Scope<ConfigReader> frame_node)
                {
                    std::pair<int, int> cell{ frame_node->as(std::pair<int, int>(0, 0)) };
                    clip.frames.push_back(cell);
                });
            }
            emplace_clip(std::move(name), std::move(clip));
        });
        return true;
    }

    bool Animation::save_to(const Path& filepath) const
    {
        file::check_to_create_dir(filepath);

        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        if(!writer) {
            CORE_ERROR(u8"Animation: Failed to create config writer!");
            return false;
        }

        writer->begin_map();
        writer->write(u8"Texture", ConfigValue(tex_uuid_.value()));

        writer->begin_array(u8"Clips");
        for(const auto& [name, clip] : clips_)
        {
            writer->begin_map();

            writer->write(u8"Name", ConfigValue(name));
            writer->write(u8"Cell Size", ConfigValue(clip.cell_size));
            writer->write(u8"FPS",  ConfigValue(clip.fps ));
            writer->write(u8"Loop", ConfigValue(clip.loop));
            writer->write(u8"Next", ConfigValue(clip.next));

            writer->begin_array(u8"Frames");
            for(const auto& cell : clip.frames)
                writer->write(ConfigValue(cell));
            writer->end_array();

            writer->end_map();
        }
        writer->end_array();
        writer->end_map();

        if(!writer->push_to_file(filepath)) {
            CORE_ERROR(u8"Animation: Failed to save '{}'!", filepath);
            return false;
        }
        return true;
    }

    std::pair<AssetHandle, bool> Animation::get_tex_handle(AssetsManager& am)
        { return am.resolve(resolved_tex_, tex_uuid_); }

    const AnimClip* Animation::get_clip(const String& name) const
    {
        if(name.empty()) {
            CORE_ERROR(u8"Animation: Clip name empty!");
            return nullptr;
        }
        auto it{ clips_.find(name) };
        if(it != clips_.end()) return &(it->second);
        CORE_ERROR(u8"Animation: Clip '{}' not found!", name);
        return nullptr;
    }

    AnimClip* Animation::get_clip_mut(const String& name)
    {
        if(name.empty()) {
            CORE_ERROR(u8"Animation: Clip name empty!");
            return nullptr;
        }
        auto it{ clips_.find(name) };
        if(it != clips_.end()) return &(it->second);
        CORE_ERROR(u8"Animation: Clip '{}' not found!", name);
        return nullptr;
    }

    void Animation::set_tex_uuid(UUID uuid) { tex_uuid_ = uuid; }

    void Animation::emplace_clip(String name, AnimClip clip)
    {
        if(name.empty()) {
            CORE_ERROR(u8"Animation: Clip name empty!");
            return;
        }
        if(clips_.contains(name)) {
            CORE_ERROR(u8"Animation: Already has clip '{}'! "
                u8"Use replace instead.", name);
            return;
        }
        clip_names_.push_back(name);
        clips_.emplace(std::move(name), std::move(clip));
    }

    void Animation::replace_clip(const String& name, AnimClip clip)
    {
        if(name.empty()) {
            CORE_ERROR(u8"Animation: Clip name empty!");
            return;
        }
        auto it{ clips_.find(name) };
        if(it != clips_.end()) { it->second = std::move(clip); return; }
        CORE_ERROR(u8"Animation: Clip name '{}' not found!", name);
    }

    void Animation::remove_clip(const String& name)
    {
        if(name.empty()) {
            CORE_ERROR(u8"Animation: Clip name empty!");
            return;
        }
        if(!clips_.contains(name)) {
            CORE_ERROR(u8"Animation: Clip '{}' not found!", name);
            return;
        }
        clips_.erase(name);
        std::erase_if(clip_names_, [name](const String& e){ return name == e; });
    }
}
