module;
module LogPanel;

import Log;
import String;
import Types;
import Application;

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
            { return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch; } };

        for(Size offset{ 0 }; offset + needle_size <= hay_size; ++offset)
        {
            bool match{ true };
            for(Size i{ 0 }; i < needle_size; ++i)
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
        if(!show_source)
            return String::format(u8"[{:%T}][{}] {}",
                zoned_time, level_name_sv(entry.level), entry.message);

        String file_name{ str::extract_filename
            (StringView(str::to_char8(entry.file ? entry.file : ""))) };
        return String::format(u8"[{:%T}][{}][{}:{}] {}",
            zoned_time, level_name_sv(entry.level), file_name, entry.line, entry.message);
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

    void LogPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());

        pull_new_entries();
        if(dirty_) rebuild_visible();

        draw_toolbar();
        draw_entries();
        draw_context_popup();

        ImGui::End();
    }

    void LogPanel::pull_new_entries()
    {
        if(log_history.written() == consumed_) return;

        consumed_ = log_history.copy_since(consumed_, entries_);
        dirty_ = true;
    }

    void LogPanel::rebuild_visible()
    {
    // rows are formatted once and cached; toggling the source column invalidates them
        if(formatted_with_source_ != settings_.show_source)
        {
            formatted_ = 0;
            formatted_with_source_ = settings_.show_source;
        }
        display_.resize(entries_.size());
        for(Size i{ formatted_ }; i < entries_.size(); ++i)
            display_[i] = build_display_text(entries_[i], settings_.show_source);
        formatted_ = entries_.size();

        visible_.clear();
        level_counts_.fill(0);
        for(Size i{ 0 }; i < entries_.size(); ++i)
        {
            const LogEntry& entry{ entries_[i] };
            ++level_counts_[static_cast<Size>(entry.level)];

            if(!filters_.levels[static_cast<Size>(entry.level)]) continue;
            if(entry.type == LogType::Core ? !filters_.core : !filters_.client) continue;
            if(!icontains(entry.message, filters_.search)) continue;

            visible_.push_back(i);
        }

        dirty_ = false;
        layout_valid_ = false; // the visible set changed: heights must be rebuilt
        if(settings_.auto_scroll) scroll_to_bottom_ = true;
    }

    void LogPanel::draw_toolbar()
    {
        const float row_height{ ImGui::GetFrameHeight() };

    // group 1: search
        ImGui::SetNextItemWidth(220.0f);
        if(ImGui::InputTextWithHint("##log_search", "search...",
            search_buffer_, sizeof(search_buffer_)))
        {
            filters_.search = String{ str::to_char8(search_buffer_) };
            dirty_ = true;
        }
        ImGui::SameLine();
        vertical_separator(row_height);

    // group 2: level toggles, colored like the rows themselves
        for(Size i{ 0 }; i < filters_.levels.size(); ++i)
        {
            const LogLevel level{ static_cast<LogLevel>(i) };
            char label[48]{};
            std::snprintf(label, sizeof(label), "%s(%zu)",
                level_name_sv(level).raw_unsafe(), static_cast<size_t>(level_counts_[i]));

            ImGui::PushStyleColor(ImGuiCol_Text, level_color(level));
            if(ImGui::Checkbox(label, &filters_.levels[i])) dirty_ = true;
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        vertical_separator(row_height);

    // group 3: senders
        if(ImGui::Checkbox("Core", &filters_.core)) dirty_ = true;
        ImGui::SameLine(0.0f, 10.0f);
        if(ImGui::Checkbox("Client", &filters_.client)) dirty_ = true;

        ImGui::Text("%zu entries, %zu shown", static_cast<size_t>(entries_.size()),
            static_cast<size_t>(visible_.size()));
        if(const uint64 recycled{ log_history.dropped() }; recycled > 0)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f),
                "(%llu older entries recycled)", static_cast<unsigned long long>(recycled));
        }
    }

    void LogPanel::rebuild_layout(float content_width, float line_height)
    {
        const Size rows{ visible_.size() };
        const float spacing{ ImGui::GetStyle().ItemSpacing.y };
        const float wrap_width{ content_width > 0.0f ? content_width : 1.0f };

        row_heights_.resize(rows);
        for(Size row{ 0 }; row < rows; ++row)
        {
            const String& text{ display_[visible_[row]] };
            // first estimate: measuring is much cheaper than drawing, and rows
            // actually drawn later correct it through their real item height
            row_heights_[row] = (settings_.wrap
                ? ImGui::CalcTextSize(text.raw(), nullptr, false, wrap_width).y
                : line_height) + spacing;
        }

        measured_width_ = content_width;
        layout_valid_ = true;
        layout_dirty_ = false;
        rebuild_offsets();
    }

    void LogPanel::rebuild_offsets()
    {
        row_offsets_.resize(row_heights_.size() + 1);

        float offset{ 0.0f };
        for(Size row{ 0 }; row < row_heights_.size(); ++row)
        {
            row_offsets_[row] = offset;
            offset += row_heights_[row];
        }
        row_offsets_[row_heights_.size()] = offset; // exact content height
    }

    void LogPanel::draw_entries()
    {
        ImGui::BeginChild("##log_entries", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        hovered_row_ = -1; // re-detected while the rows below are submitted

        const float content_width{ ImGui::GetContentRegionAvail().x };
        if(!layout_valid_ || measured_width_ != content_width)
            rebuild_layout(content_width, ImGui::GetTextLineHeight());

    // keep following the tail, but never yank the view while the user reads history
        const bool follow {
            settings_.auto_scroll &&
            ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f
        };

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
                if(row_offsets_[mid] + row_heights_[mid] < view_top) low = mid + 1;
                else high = mid;
            }
            first = low;
        }

        for(Size row{ first }; row < rows; ++row)
        {
            if(row_offsets_[row] > view_bottom) break; // starts below the viewport

            ImGui::SetCursorPosY(row_offsets_[row]);
            const float real_height{ draw_entry(static_cast<int>(row))
                + ImGui::GetStyle().ItemSpacing.y };

            float delta{ real_height - row_heights_[row] };
            if(delta < 0.0f) delta = -delta;
            if(delta > 0.5f)
            {
                row_heights_[row] = real_height;
                layout_dirty_ = true;
            }
        }
        if(layout_dirty_) { rebuild_offsets(); layout_dirty_ = false; }

    // anchor the content height, so the scrollbar reaches the very last row
        ImGui::SetCursorPosY(row_offsets_.empty() ? 0.0f : row_offsets_.back());
        ImGui::Dummy(ImVec2(0.0f, 0.0f));

        if(follow || scroll_to_bottom_) ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom_ = false;

        ImGui::EndChild();
    }

    void LogPanel::draw_context_popup()
    {
        if(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
        && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            context_row_ = hovered_row_;
            ImGui::OpenPopup("##log_context");
        }

        if(!ImGui::BeginPopup("##log_context")) return;

        if(context_row_ >= 0 && static_cast<Size>(context_row_) < visible_.size())
        {
            const Size index{ visible_[static_cast<Size>(context_row_)] };
            if(ImGui::MenuItem("Copy Line"))    ImGui::SetClipboardText(display_[index].raw());
            if(ImGui::MenuItem("Copy Message")) ImGui::SetClipboardText(entries_[index].message.raw());
            ImGui::Separator();
        }

        if(ImGui::MenuItem("Copy All"))
        {
            String joined{};
            for(Size index : visible_)
            {
                joined += display_[index];
                joined += u8"\n";
            }
            if(!joined.empty()) ImGui::SetClipboardText(joined.raw());
        }
        if(ImGui::MenuItem("Clear")) clear_entries();
        ImGui::Separator();

        ImGui::MenuItem("Auto-scroll",   nullptr, &settings_.auto_scroll);
        if(ImGui::MenuItem("Wrap",        nullptr, &settings_.wrap)) layout_valid_ = false;
        if(ImGui::MenuItem("Show Source", nullptr, &settings_.show_source)) dirty_ = true;

        ImGui::EndPopup();
    }

    float LogPanel::draw_entry(int row)
    {
        const Size index{ visible_[static_cast<Size>(row)] };
        const String& text{ display_[index] };

        ImGui::PushStyleColor(ImGuiCol_Text, level_color(entries_[index].level));
        if(settings_.wrap) ImGui::TextWrapped("%s", text.raw());
        else ImGui::TextUnformatted(text.raw());
        ImGui::PopStyleColor();

        const float height{ ImGui::GetItemRectSize().y };

    // hover must still register while the context popup is open, otherwise the
    // popup could not tell which row it was opened on
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            hovered_row_ = row;

        return height;
    }

    void LogPanel::clear_entries()
    {
        log_history.clear();

        entries_.clear();
        display_.clear();
        visible_.clear();
        level_counts_.fill(0);

        consumed_  = log_history.written();
        formatted_ = 0;
        dirty_     = true;
        layout_valid_ = false;
        scroll_to_bottom_ = true;
    }
}
