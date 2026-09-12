module;
module LogPanel;

import FileUtils;
import ConfigProxy;

namespace {
    using namespace rke;

    constexpr StringView level_name_sv(LogLevel level)
    {
        using namespace rke::literals;
        switch(level)
        {
        case LogLevel::Info:     return u8"Info"_sv;
        case LogLevel::Warn:     return u8"Warn"_sv;
        case LogLevel::Error:    return u8"Error"_sv;
        case LogLevel::Trace:    return u8"Trace"_sv;
        case LogLevel::Critical: return u8"Critical"_sv;
        }
        return u8"Unknown"_sv;
    }

    constexpr ImVec4 level_color(LogLevel level)
    {
        switch(level)
        {
        case LogLevel::Info:     return ImVec4(0.55f, 0.85f, 0.55f, 1.0f);
        case LogLevel::Warn:     return ImVec4(0.95f, 0.85f, 0.40f, 1.0f);
        case LogLevel::Error:    return ImVec4(0.95f, 0.45f, 0.45f, 1.0f);
        case LogLevel::Trace:    return ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
        case LogLevel::Critical: return ImVec4(1.00f, 0.35f, 0.35f, 1.0f);
        }
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    static bool icontains(StringView haystack, StringView needle)
    {
        if(needle.empty()) return true;
        const Size hay_size{ haystack.size() };
        const Size needle_size{ needle.size() };
        if(needle_size > hay_size) return false;

        const char* hay{ haystack.raw_unsafe() };
        const char* pin{ needle.raw_unsafe() };
        const auto lower{ [](char ch) -> char
        {
            return (ch >= 'A' && ch <= 'Z') ?
                static_cast<char>(ch - 'A' + 'a') : ch;
        }};

        for(Size offset{}; offset + needle_size <= hay_size; ++offset)
        {
            bool match{ true };
            for(Size i{}; i < needle_size; ++i)
            {
                if(lower(hay[offset + i]) == lower(pin[i])) continue;
                match = false;
                break;
            }
            if(match) return true;
        }
        return false;
    }

    static String build_display_text(const LogEntry& entry, bool show_source)
    {
        auto zoned_time{ std::chrono::zoned_time(std::chrono::current_zone(), entry.time) };
        if(!show_source) return String::format(u8"[{:%T}] {}", zoned_time, entry.message);

        String file_name{ str::extract_filename
            (StringView(str::to_char8(entry.file ? entry.file : ""))) };
        return String::format(u8"[{:%T}][{}:{}] {}",
            zoned_time, file_name, entry.line, entry.message);
    }

    static void vertical_separator(float height)
    {
        const ImVec2 top{ ImGui::GetCursorScreenPos() };

        ImGui::Dummy(ImVec2(1.0f, height));
        ImGui::GetWindowDrawList()->AddLine(top, ImVec2(top.x, top.y + height),
            ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
    }
}

namespace rke
{
    extern LogHistory log_history;

    LogPanel::~LogPanel()
    {
        if(filepath_.empty()) return;
        file::check_to_create_dir(filepath_);

        auto writer{ ConfigWriter::create() };
        if(!writer) {
            CORE_ERROR(u8"LogPanel: Failed to create config writer!");
            return;
        }
        writer->begin_map();
        writer->write(u8"Auto Scroll", settings_.auto_scroll);
        writer->write(u8"Wrap", settings_.wrap);
        writer->write(u8"Show Source", settings_.show_source);
        writer->end_map();

        writer->push_to_file(filepath_);
    }

    void LogPanel::load_from(Path filepath)
    {
        filepath_ = std::move(filepath);
        if(!filepath_.exists()) {
            CORE_WARN(u8"LogPanel: File '{}' not found!", filepath_);
            return;
        }
        auto reader{ ConfigReader::create(filepath_) };
        if(!reader || !reader->is_map()) {
            CORE_WARN(u8"LogPanel: File format incorrect!");
            return;
        }
        settings_.auto_scroll = reader->get_at(u8"Auto Scroll", settings_.auto_scroll);
        settings_.wrap = reader->get_at(u8"Wrap", settings_.wrap);
        settings_.show_source = reader->get_at(u8"Show Source", settings_.show_source);
    }

    void LogPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());

        pull_new_entries();
        if(visible_dirty_) rebuild_visible();

        draw_toolbar();
        draw_entries();
        draw_context_popup();

        ImGui::End();
    }

    void LogPanel::pull_new_entries()
    {
        if(log_history.written() == consumed_) return;
        consumed_ = log_history.copy_since(consumed_, entries_);
        visible_dirty_ = true;
    }

    void LogPanel::rebuild_visible()
    {
    // the text and the heights depend on 'show source', so a change drops both
        if(formatted_with_source_ != settings_.show_source)
        {
            formatted_with_source_ = settings_.show_source;
            formatted_.clear();
            formatted_heights_.clear();
        }
        formatted_.reserve(entries_.size()); // only triggers when size is smaller
        while(formatted_.size() < entries_.size())
            formatted_.push_back(build_display_text
                (entries_[formatted_.size()], settings_.show_source));

        visible_.clear();
        level_counts_.fill(0);
        const StringView search{ str::to_char8(search_buffer_) };
        for(Size i{}; i < entries_.size(); ++i)
        {
            const LogEntry& entry{ entries_[i] };
            level_counts_[static_cast<Size>(entry.level)]++;

            if(!filters_.levels[static_cast<Size>(entry.level)]) continue;
            if(entry.type == LogType::Core ? !filters_.core : !filters_.client) continue;
            if(!icontains(entry.message, search)) continue;

            visible_.push_back(i);
        }
        if(settings_.auto_scroll) scroll_to_bottom_ = true;
        visible_dirty_ = false;
    }

    void LogPanel::measure_heights()
    {
        const float wrap_width{ measured_width_ > 0.0f ? measured_width_ : 1.0f };

        formatted_heights_.reserve(formatted_.size()); // the same as above
        while(formatted_heights_.size() < formatted_.size())
        {
            const Size index{ formatted_heights_.size() };
            formatted_heights_.push_back((settings_.wrap
                ? ImGui::CalcTextSize(formatted_[index].raw(), nullptr, false, wrap_width).y
                : ImGui::GetTextLineHeight()) + ImGui::GetStyle().ItemSpacing.y);
        }
    }

    void LogPanel::rebuild_offsets()
    {
        row_offsets_.resize(visible_.size() + 1);

        float offset{ 0.0f };
        for(Size row{}; row < visible_.size(); ++row)
        {
            row_offsets_[row] = offset;
            offset += formatted_heights_[visible_[row]];
        }
        row_offsets_[visible_.size()] = offset; // exact content height
    }

    void LogPanel::draw_toolbar()
    {
        const float row_height{ ImGui::GetFrameHeight() };

    // group 1: search
        ImGui::SetNextItemWidth(220.0f);
        if(ImGui::InputTextWithHint("##log_search", "search...",
            search_buffer_, sizeof(search_buffer_)))
            visible_dirty_ = true;
        ImGui::SameLine();
        vertical_separator(row_height);

    // group 2: senders
        if(ImGui::Checkbox("Core", &filters_.core)) visible_dirty_ = true;
        ImGui::SameLine(0.0f, 10.0f);
        if(ImGui::Checkbox("Client", &filters_.client)) visible_dirty_ = true;
        ImGui::SameLine();
        vertical_separator(row_height);

    // group 3: level toggles, colored like the rows themselves
        for(Size i{ 0 }; i < filters_.levels.size(); ++i)
        {
            const LogLevel level{ static_cast<LogLevel>(i) };
            char label[48]{};
            std::snprintf(label, sizeof(label), "%s(%zu)",
                level_name_sv(level).raw_unsafe(), static_cast<size_t>(level_counts_[i]));

            ImGui::PushStyleColor(ImGuiCol_Text, level_color(level));
            if(i) ImGui::SameLine();
            if(ImGui::Checkbox(label, &filters_.levels[i])) visible_dirty_ = true;
            ImGui::PopStyleColor();
        }
        
        ImGui::Text("%zu entries, %zu shown", entries_.size(), visible_.size());
        const uint64 recycled{ log_history.dropped() };
        if(recycled > 0)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f),
                "(%llu older entries recycled)", recycled);
        }
    }

    void LogPanel::draw_entries()
    {
        ImGui::BeginChild("##log_entries", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        if(!ImGui::IsPopupOpen("##log_context")) context_row_ = -1;

        const float content_width{ ImGui::GetContentRegionAvail().x };
        if(measured_width_ != content_width || measured_wrapped_ != settings_.wrap)
        {
            measured_width_   = content_width;
            measured_wrapped_ = settings_.wrap;
            formatted_heights_.clear(); // heights depend on the wrapping context
        }
        measure_heights();
        rebuild_offsets();

    // keep following the tail, but never yank the view while the user reads history
        const bool at_bottom{ ImGui::GetScrollY() >= (ImGui::GetScrollMaxY() - 1.0f) };

        const Size rows{ visible_.size() };
        const float view_top{ ImGui::GetScrollY() };
        const float view_bottom{ view_top + ImGui::GetWindowHeight() };

    // first row that may be visible: binary search over the prefix sums
        Size first{ 0 };
        {
            Size low{ 0 }, high{ rows };
            while(low < high)
            {
                const Size mid{ low + (high - low) / 2 };
                if(row_offsets_[mid] + formatted_heights_[visible_[mid]] < view_top) low = mid + 1;
                else high = mid;
            }
            first = low;
        }

        bool corrected{ false };
        for(Size row{ first }; row < rows; ++row)
        {
            if(row_offsets_[row] > view_bottom) break; // starts below the viewport

            ImGui::SetCursorPosY(row_offsets_[row]);
            const float real_height{ draw_entry(static_cast<int>(row))
                + ImGui::GetStyle().ItemSpacing.y };

        // the estimate is replaced by what the row really took
            const Size index{ visible_[row] };
            float delta{ real_height - formatted_heights_[index] };
            if(delta < 0.0f) delta = -delta;
            if(delta > 0.5f)
            {
                formatted_heights_[index] = real_height;
                corrected = true;
            }
        }
        if(corrected) rebuild_offsets();

    // anchor the content height, so the scrollbar reaches the very last row
        ImGui::SetCursorPosY(row_offsets_.empty() ? 0.0f : row_offsets_.back());
        ImGui::Dummy(ImVec2(0.0f, 0.0f));

        if(at_bottom || scroll_to_bottom_) ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom_ = false;

        ImGui::EndChild();
    }

    void LogPanel::draw_context_popup()
    {
        if(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
        && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup("##log_context"); // the row, if any, set context_row_

        if(!ImGui::BeginPopup("##log_context")) return;

        if(context_row_ >= 0 && static_cast<Size>(context_row_) < visible_.size())
        {
            const Size index{ visible_[static_cast<Size>(context_row_)] };
            if(ImGui::MenuItem("Copy Line"))    ImGui::SetClipboardText(formatted_[index].raw());
            if(ImGui::MenuItem("Copy Message")) ImGui::SetClipboardText(entries_[index].message.raw());
            ImGui::Separator();
        }

        if(ImGui::MenuItem("Copy All"))
        {
            String joined{};
            for(Size index : visible_)
            {
                joined += formatted_[index];
                joined += u8"\n";
            }
            if(!joined.empty()) ImGui::SetClipboardText(joined.raw());
        }
        if(ImGui::MenuItem("Clear")) clear_entries();
        ImGui::Separator();

        ImGui::MenuItem("Auto-scroll", nullptr, &settings_.auto_scroll);
        ImGui::MenuItem("Wrap", nullptr, &settings_.wrap); // picked up by draw_entries
        if(ImGui::MenuItem("Show Source", nullptr, &settings_.show_source)) visible_dirty_ = true;

        ImGui::EndPopup();
    }

    float LogPanel::draw_entry(int row)
    {
        const Size index{ visible_[static_cast<Size>(row)] };
        const String& text{ formatted_[index] };

        ImGui::PushStyleColor(ImGuiCol_Text, level_color(entries_[index].level));
        if(settings_.wrap) ImGui::TextWrapped("%s", text.raw());
        else ImGui::TextUnformatted(text.raw());
        ImGui::PopStyleColor();

        const float height{ ImGui::GetItemRectSize().y };

    // a right click here selects this row for the context popup; hover must still
    // register while a popup is open, or the row could not be picked
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)
        && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            context_row_ = row;

        return height;
    }

    void LogPanel::clear_entries()
    {
        log_history.clear();

        entries_.clear();
        consumed_ = log_history.written();

        formatted_.clear();
        formatted_heights_.clear();

        visible_.clear();

        level_counts_.fill(0);
        measured_width_ = -1.0f;
        measured_wrapped_ = settings_.wrap;
        scroll_to_bottom_ = true;
    }
}
